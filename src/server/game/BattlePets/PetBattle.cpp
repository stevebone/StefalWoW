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

#include "PetBattle.h"
#include "BattlePetMgr.h"
#include "BattlePetPackets.h"
#include "Creature.h"
#include "DB2Stores.h"
#include "GameTables.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "PetBattleMgr.h"
#include "Player.h"
#include "Random.h"
#include "WorldSession.h"
#include <algorithm>

namespace PetBattles
{

PetBattle::PetBattle()
{
    _teams = {};
    _environments = {};
}

PetBattle::~PetBattle() = default;

// ============================================================================
// Team loading helpers
// ============================================================================

void PetBattle::LoadPlayerTeam(Player* player, PetBattleTeamData& team)
{
    team.PlayerGUID = player->GetGUID();

    BattlePets::BattlePetMgr* petMgr = player->GetSession()->GetBattlePetMgr();
    uint8 petIdx = 0;
    for (uint8 i = 0; i < uint8(BattlePets::BattlePetSlot::Count); ++i)
    {
        WorldPackets::BattlePet::BattlePetSlot* slot = petMgr->GetSlot(BattlePets::BattlePetSlot(i));
        if (!slot || slot->Locked)
            continue;

        BattlePets::BattlePet* pet = petMgr->GetPet(slot->Pet.Guid);
        if (!pet || pet->PacketInfo.Health == 0)
            continue;

        PetBattlePetData& battlePet = team.Pets[petIdx];
        battlePet.BattlePetGUID = pet->PacketInfo.Guid;
        battlePet.Species = pet->PacketInfo.Species;
        battlePet.CreatureID = pet->PacketInfo.CreatureID;
        battlePet.DisplayID = pet->PacketInfo.DisplayID;
        battlePet.Breed = pet->PacketInfo.Breed;
        battlePet.Level = pet->PacketInfo.Level;
        battlePet.Xp = pet->PacketInfo.Exp;
        battlePet.Quality = pet->PacketInfo.Quality;
        battlePet.Health = pet->PacketInfo.Health;
        battlePet.MaxHealth = pet->PacketInfo.MaxHealth;
        battlePet.Power = pet->PacketInfo.Power;
        battlePet.Speed = pet->PacketInfo.Speed;
        battlePet.EffectivePower = battlePet.Power;
        battlePet.EffectiveSpeed = battlePet.Speed;
        battlePet.CustomName = pet->PacketInfo.Name;

        BattlePetSpeciesEntry const* species = sBattlePetSpeciesStore.LookupEntry(pet->PacketInfo.Species);
        if (species)
            battlePet.PetType = species->PetTypeEnum;

        // Load abilities for this pet from DB2
        std::vector<BattlePetSpeciesXAbilityEntry const*> const* speciesAbilities =
            sPetBattleMgr->GetSpeciesAbilitiesFull(battlePet.Species);
        if (speciesAbilities)
        {
            uint8 abilitySlot = 0;
            for (BattlePetSpeciesXAbilityEntry const* entry : *speciesAbilities)
            {
                if (entry->RequiredLevel > battlePet.Level)
                    continue;
                if (entry->SlotEnum < 0 || entry->SlotEnum >= int8(MAX_PET_BATTLE_ABILITIES))
                    continue;

                uint8 slotIdx = static_cast<uint8>(entry->SlotEnum);
                battlePet.AbilityIDs[slotIdx] = entry->BattlePetAbilityID;
                ++abilitySlot;
            }
        }

        ++petIdx;
        if (petIdx >= MAX_PET_BATTLE_TEAM_SIZE)
            break;
    }
    team.PetCount = petIdx;
}

void PetBattle::LoadWildPetAbilities(PetBattlePetData& pet)
{
    std::vector<BattlePetSpeciesXAbilityEntry const*> const* speciesAbilities =
        sPetBattleMgr->GetSpeciesAbilitiesFull(pet.Species);
    if (!speciesAbilities)
        return;

    for (BattlePetSpeciesXAbilityEntry const* entry : *speciesAbilities)
    {
        if (entry->RequiredLevel > pet.Level)
            continue;
        if (entry->SlotEnum < 0 || entry->SlotEnum >= int8(MAX_PET_BATTLE_ABILITIES))
            continue;

        uint8 slotIdx = static_cast<uint8>(entry->SlotEnum);
        pet.AbilityIDs[slotIdx] = entry->BattlePetAbilityID;
    }
}

static void CalculateWildPetStats(PetBattlePetData& wildPet);

void PetBattle::GenerateWildTeam(Player* player, ObjectGuid wildCreatureGUID)
{
    PetBattleTeamData& wildTeam = _teams[PET_BATTLE_TEAM_2];

    Creature* creature = ObjectAccessor::GetCreature(*player, wildCreatureGUID);
    if (!creature)
        return;

    // Determine wild team size: 1-3 pets based on randomness
    uint8 wildTeamSize = static_cast<uint8>(urand(1, 3));

    // First pet is always the targeted creature
    {
        PetBattlePetData& wildPet = wildTeam.Pets[0];
        wildPet.CreatureID = creature->GetEntry();
        wildPet.DisplayID = creature->GetDisplayId();
        wildPet.Level = static_cast<uint16>(creature->GetWildBattlePetLevel());

        if (BattlePetSpeciesEntry const* species = BattlePets::BattlePetMgr::GetBattlePetSpeciesByCreature(creature->GetEntry()))
        {
            wildPet.Species = species->ID;
            wildPet.PetType = species->PetTypeEnum;
            wildPet.Breed = BattlePets::BattlePetMgr::RollPetBreed(species->ID);
            wildPet.Quality = uint8(BattlePets::BattlePetMgr::GetDefaultPetQuality(species->ID));
        }

        CalculateWildPetStats(wildPet);
        LoadWildPetAbilities(wildPet);
    }

    // Additional wild pets (same species, slight level variation)
    for (uint8 i = 1; i < wildTeamSize; ++i)
    {
        PetBattlePetData& wildPet = wildTeam.Pets[i];
        wildPet.CreatureID = creature->GetEntry();
        wildPet.DisplayID = creature->GetDisplayId();

        // Level varies by +/- 1 from primary
        int16 baseLevel = static_cast<int16>(creature->GetWildBattlePetLevel());
        int16 levelVariation = static_cast<int16>(urand(0, 2)) - 1; // -1, 0, or +1
        wildPet.Level = static_cast<uint16>(std::max(int16(1), int16(baseLevel + levelVariation)));

        if (BattlePetSpeciesEntry const* species = BattlePets::BattlePetMgr::GetBattlePetSpeciesByCreature(creature->GetEntry()))
        {
            wildPet.Species = species->ID;
            wildPet.PetType = species->PetTypeEnum;
            wildPet.Breed = BattlePets::BattlePetMgr::RollPetBreed(species->ID);
            wildPet.Quality = uint8(BattlePets::BattlePetMgr::GetDefaultPetQuality(species->ID));
        }

        CalculateWildPetStats(wildPet);
        LoadWildPetAbilities(wildPet);
    }

    wildTeam.PetCount = wildTeamSize;
}

static void CalculateWildPetStats(PetBattlePetData& wildPet)
{
    float qualityMultiplier = 1.0f;
    for (BattlePetBreedQualityEntry const* entry : sBattlePetBreedQualityStore)
        if (entry->QualityEnum == wildPet.Quality)
        {
            qualityMultiplier = entry->StateMultiplier;
            break;
        }

    int32 baseHP = 100, basePower = 10, baseSpeed = 10;
    for (BattlePetBreedStateEntry const* entry : sBattlePetBreedStateStore)
    {
        if (entry->BattlePetBreedID != wildPet.Breed)
            continue;
        if (entry->BattlePetStateID == BattlePets::STATE_STAT_STAMINA)
            baseHP = entry->Value;
        else if (entry->BattlePetStateID == BattlePets::STATE_STAT_POWER)
            basePower = entry->Value;
        else if (entry->BattlePetStateID == BattlePets::STATE_STAT_SPEED)
            baseSpeed = entry->Value;
    }

    wildPet.MaxHealth = int32((baseHP * 5.0f * wildPet.Level + 100.0f) * qualityMultiplier);
    wildPet.Health = wildPet.MaxHealth;
    wildPet.Power = int32((basePower * wildPet.Level / 20.0f) * qualityMultiplier);
    wildPet.Speed = int32((baseSpeed * wildPet.Level / 20.0f) * qualityMultiplier);
    wildPet.EffectivePower = wildPet.Power;
    wildPet.EffectiveSpeed = wildPet.Speed;

    if (wildPet.Power < 1) wildPet.Power = 1;
    if (wildPet.Speed < 1) wildPet.Speed = 1;
    if (wildPet.EffectivePower < 1) wildPet.EffectivePower = 1;
    if (wildPet.EffectiveSpeed < 1) wildPet.EffectiveSpeed = 1;
}

// ============================================================================
// Battle initialization
// ============================================================================

void PetBattle::InitWildBattle(Player* player, ObjectGuid wildCreatureGUID)
{
    _battleType = PET_BATTLE_TYPE_PVE;
    _wildCreatureGUID = wildCreatureGUID;
    _canAwardXP = true;

    LoadPlayerTeam(player, _teams[PET_BATTLE_TEAM_1]);
    GenerateWildTeam(player, wildCreatureGUID);

    // Recalculate effective stats for all pets
    for (uint8 t = 0; t < MAX_PET_BATTLE_PLAYERS; ++t)
        for (uint8 p = 0; p < _teams[t].PetCount; ++p)
            _teams[t].Pets[p].RecalculateEffectiveStats();

    _state = PET_BATTLE_STATE_WAITING_PRE_BATTLE;
}

void PetBattle::InitPvPBattle(Player* player1, Player* player2)
{
    _battleType = PET_BATTLE_TYPE_PVP;
    _canAwardXP = false;

    LoadPlayerTeam(player1, _teams[PET_BATTLE_TEAM_1]);
    LoadPlayerTeam(player2, _teams[PET_BATTLE_TEAM_2]);

    // Validate both teams have at least 1 alive pet
    if (!_teams[PET_BATTLE_TEAM_1].HasAlivePets() || !_teams[PET_BATTLE_TEAM_2].HasAlivePets())
    {
        _state = PET_BATTLE_STATE_CREATED_FAILED;
        return;
    }

    // Recalculate effective stats for all pets
    for (uint8 t = 0; t < MAX_PET_BATTLE_PLAYERS; ++t)
        for (uint8 p = 0; p < _teams[t].PetCount; ++p)
            _teams[t].Pets[p].RecalculateEffectiveStats();

    _state = PET_BATTLE_STATE_WAITING_PRE_BATTLE;
}

// ============================================================================
// State machine
// ============================================================================

void PetBattle::Start()
{
    _state = PET_BATTLE_STATE_ROUND_IN_PROGRESS;
    _currentRound = 0;
}

void PetBattle::Update(uint32 diff)
{
    if (_state == PET_BATTLE_STATE_FINISHED)
        return;

    _updateTimer += diff;
    if (_updateTimer >= 1000)
    {
        _elapsedSecs += _updateTimer / 1000;
        _updateTimer %= 1000;

        // 5 minute warning before timeout
        if (!_maxLengthWarningSent && _elapsedSecs >= PET_BATTLE_MAX_GAME_LENGTH - 300)
        {
            _maxLengthWarningSent = true;
            for (uint8 i = 0; i < MAX_PET_BATTLE_PLAYERS; ++i)
            {
                if (Player* player = GetPlayerForTeam(i))
                {
                    WorldPackets::BattlePet::PetBattleMaxGameLengthWarning warning;
                    player->SendDirectMessage(warning.Write());
                }
            }
        }

        if (_elapsedSecs >= PET_BATTLE_MAX_GAME_LENGTH)
        {
            FinishBattle(PET_BATTLE_RESULT_DRAW);
            return;
        }
    }

    if (_state == PET_BATTLE_STATE_ROUND_IN_PROGRESS && BothTeamsReady())
        ProcessRound();
}

void PetBattle::SubmitInput(uint8 teamIdx, PetBattleMoveType moveType, uint32 abilityID, int8 newFrontPet)
{
    if (teamIdx >= MAX_PET_BATTLE_PLAYERS)
        return;

    PetBattleTeamData& team = _teams[teamIdx];
    team.PendingMoveType = moveType;
    team.PendingAbilityID = abilityID;
    team.PendingNewFrontPet = newFrontPet;
    team.HasInputThisRound = true;
}

bool PetBattle::BothTeamsReady() const
{
    return _teams[PET_BATTLE_TEAM_1].HasInputThisRound &&
           _teams[PET_BATTLE_TEAM_2].HasInputThisRound;
}

// ============================================================================
// Round processing - the core combat loop
// ============================================================================

void PetBattle::ProcessRound()
{
    _state = PET_BATTLE_STATE_ROUND_IN_PROGRESS;
    _currentRound++;
    _roundEffects.clear();
    _petKilledThisRound.fill(false);
    _needsFrontPetSwap.fill(false);

    // Recalculate effective stats for all living pets (includes passive bonuses)
    for (uint8 t = 0; t < MAX_PET_BATTLE_PLAYERS; ++t)
        for (uint8 p = 0; p < _teams[t].PetCount; ++p)
            if (_teams[t].Pets[p].IsAlive())
                _teams[t].Pets[p].RecalculateEffectiveStats();

    // Apply passive round-start effects (Humanoid heal, Dragonkin reset, etc.)
    ApplyPassiveRoundStart();

    // Determine turn order based on effective speed
    uint8 firstTeam = PET_BATTLE_TEAM_1;
    uint8 secondTeam = PET_BATTLE_TEAM_2;

    PetBattlePetData& pet1 = _teams[PET_BATTLE_TEAM_1].Pets[_teams[PET_BATTLE_TEAM_1].FrontPetIndex];
    PetBattlePetData& pet2 = _teams[PET_BATTLE_TEAM_2].Pets[_teams[PET_BATTLE_TEAM_2].FrontPetIndex];

    if (pet2.EffectiveSpeed > pet1.EffectiveSpeed)
        std::swap(firstTeam, secondTeam);
    else if (pet2.EffectiveSpeed == pet1.EffectiveSpeed)
    {
        // Speed tie: random 50/50
        if (urand(0, 1))
            std::swap(firstTeam, secondTeam);
    }

    // Process turns in speed order
    ProcessTurnForTeam(firstTeam);
    if (!IsFinished())
        ProcessTurnForTeam(secondTeam);

    if (IsFinished())
        return;

    // Tick auras (DoTs, HoTs, cooldown reduction, expiry)
    TickAuras();

    // Tick weather effects
    TickWeather();

    // Decrease ability cooldowns and lockdowns
    for (auto& team : _teams)
    {
        for (uint8 p = 0; p < team.PetCount; ++p)
        {
            for (uint8 a = 0; a < MAX_PET_BATTLE_ABILITIES; ++a)
            {
                if (team.Pets[p].AbilityCooldowns[a] > 0)
                    --team.Pets[p].AbilityCooldowns[a];
                if (team.Pets[p].AbilityLockdowns[a] > 0)
                    --team.Pets[p].AbilityLockdowns[a];
            }
        }
    }

    // Check for deaths (with passive resurrection handling)
    CheckDeaths();

    // Reset input flags
    for (auto& team : _teams)
        team.HasInputThisRound = false;

    // Check if battle should end
    if (!_teams[PET_BATTLE_TEAM_1].HasAlivePets())
    {
        FinishBattle(PET_BATTLE_RESULT_TEAM_2_WIN);
        return;
    }
    if (!_teams[PET_BATTLE_TEAM_2].HasAlivePets())
    {
        FinishBattle(PET_BATTLE_RESULT_TEAM_1_WIN);
        return;
    }

    // Check if any team needs to swap front pet (front pet died)
    for (uint8 i = 0; i < MAX_PET_BATTLE_PLAYERS; ++i)
    {
        if (!_teams[i].Pets[_teams[i].FrontPetIndex].IsAlive())
        {
            _needsFrontPetSwap[i] = true;
            _state = PET_BATTLE_STATE_WAITING_FOR_FRONT_PET;

            // Wild/NPC team auto-swaps
            if (i == PET_BATTLE_TEAM_2 && (_battleType == PET_BATTLE_TYPE_PVE || _battleType == PET_BATTLE_TYPE_NPC))
            {
                int8 nextAlive = _teams[i].GetFirstAlivePetIndex();
                if (nextAlive >= 0)
                {
                    _teams[i].FrontPetIndex = nextAlive;
                    _needsFrontPetSwap[i] = false;
                }
            }
            if (_needsFrontPetSwap[i])
                return;
        }
    }

    // Compute InputFlags per team for the next round
    for (uint8 i = 0; i < MAX_PET_BATTLE_PLAYERS; ++i)
    {
        PetBattleTeamData& team = _teams[i];
        team.InputFlags = 0;

        PetBattlePetData const& frontPet = team.Pets[team.FrontPetIndex];
        if (frontPet.IsLockedByMultiTurn)
            team.InputFlags |= PET_BATTLE_INPUT_FLAG_ABILITY_LOCKED;
        if (frontPet.IsStunned)
            team.InputFlags |= PET_BATTLE_INPUT_FLAG_SWAP_LOCKED;
    }

    _state = PET_BATTLE_STATE_ROUND_IN_PROGRESS;
}

void PetBattle::ProcessTurnForTeam(uint8 teamIdx)
{
    PetBattleTeamData& team = _teams[teamIdx];
    uint8 opponentTeam = teamIdx == PET_BATTLE_TEAM_1 ? PET_BATTLE_TEAM_2 : PET_BATTLE_TEAM_1;
    PetBattlePetData& activePet = team.Pets[team.FrontPetIndex];

    // Stunned pets skip their turn
    if (activePet.IsStunned)
    {
        activePet.IsStunned = false;
        return;
    }

    // Multi-turn ability: auto-continue on subsequent turns
    if (activePet.IsLockedByMultiTurn && activePet.MultiTurnAbilityID != 0)
    {
        ApplyAbilityEffects(teamIdx, team.FrontPetIndex, activePet.MultiTurnAbilityID);
        return;
    }

    // Undead pet in revive round: dies at end of round
    if (activePet.IsUndeadReviving)
    {
        activePet.IsUndeadReviving = false;
        activePet.Health = 0;
        _petKilledThisRound[teamIdx * MAX_PET_BATTLE_TEAM_SIZE + team.FrontPetIndex] = true;
        return;
    }

    switch (team.PendingMoveType)
    {
        case PET_BATTLE_MOVE_ABILITY:
        {
            if (!activePet.IsAlive())
                break;

            // Check ability cooldown
            bool abilityOnCooldown = false;
            for (uint8 i = 0; i < MAX_PET_BATTLE_ABILITIES; ++i)
            {
                if (activePet.AbilityIDs[i] == team.PendingAbilityID)
                {
                    if (activePet.AbilityCooldowns[i] > 0)
                    {
                        abilityOnCooldown = true;
                        break;
                    }

                    // Apply cooldown from DB2
                    if (BattlePetAbilityEntry const* ability = sBattlePetAbilityStore.LookupEntry(team.PendingAbilityID))
                        activePet.AbilityCooldowns[i] = ability->Cooldown;
                    break;
                }
            }

            if (abilityOnCooldown)
                break;

            // Apply ability effects through the DB2 chain
            ApplyAbilityEffects(teamIdx, team.FrontPetIndex, team.PendingAbilityID);
            break;
        }
        case PET_BATTLE_MOVE_SWAP:
        {
            if (team.PendingNewFrontPet >= 0 && team.PendingNewFrontPet < int8(team.PetCount))
            {
                if (team.Pets[team.PendingNewFrontPet].IsAlive())
                {
                    team.FrontPetIndex = team.PendingNewFrontPet;

                    PetBattleRoundEffect effect;
                    effect.EffectType = PET_BATTLE_EFFECT_PET_SWAP;
                    effect.SourceTeam = teamIdx;
                    effect.SourcePet = team.PendingNewFrontPet;
                    _roundEffects.push_back(effect);
                }
            }
            break;
        }
        case PET_BATTLE_MOVE_TRAP:
        {
            uint8 trapStatus = GetTrapStatus(teamIdx);
            if (trapStatus != PET_BATTLE_TRAP_STATUS_CAN_TRAP)
                break;

            PetBattleTeamData& wildTeam = _teams[PET_BATTLE_TEAM_2];
            PetBattlePetData& wildPet = wildTeam.Pets[wildTeam.FrontPetIndex];

            float healthPct = wildPet.MaxHealth > 0 ? (float(wildPet.Health) / float(wildPet.MaxHealth)) * 100.0f : 100.0f;

            uint16 trapLevel = 1;
            if (Player* player = GetPlayerForTeam(teamIdx))
                trapLevel = player->GetSession()->GetBattlePetMgr()->GetTrapLevel();

            float captureChance = GetCaptureChance(trapLevel, healthPct);

            if (frand(0.0f, 1.0f) < captureChance)
            {
                wildPet.IsCaptured = true;
                wildPet.Health = 0;

                // Achievement: pet obtained through battle
                if (Player* player = GetPlayerForTeam(teamIdx))
                    player->UpdateCriteria(CriteriaType::AccountObtainPetThroughBattle, wildPet.Species);

                FinishBattle(PET_BATTLE_RESULT_TEAM_1_WIN);
            }
            break;
        }
        case PET_BATTLE_MOVE_QUIT:
        {
            Forfeit(teamIdx);
            break;
        }
        case PET_BATTLE_MOVE_PASS:
        case PET_BATTLE_MOVE_FINAL_ROUND_OK:
        default:
            break;
    }
}

// ============================================================================
// DB2-driven ability effect chain
// ============================================================================

void PetBattle::ApplyAbilityEffects(uint8 attackerTeam, uint8 attackerPet, uint32 abilityID)
{
    PetBattlePetData& attacker = _teams[attackerTeam].Pets[attackerPet];
    uint8 defenderTeam = attackerTeam == PET_BATTLE_TEAM_1 ? PET_BATTLE_TEAM_2 : PET_BATTLE_TEAM_1;
    uint8 defenderPet = _teams[defenderTeam].FrontPetIndex;

    // Get ability turns from DB2 index
    std::vector<BattlePetAbilityTurnEntry const*> const* turns = sPetBattleMgr->GetAbilityTurnsFull(abilityID);
    if (!turns || turns->empty())
    {
        // Fallback: apply simple damage if no DB2 turn data exists
        PetBattlePetData& defender = _teams[defenderTeam].Pets[defenderPet];
        if (attacker.IsAlive() && defender.IsAlive())
        {
            BattlePetAbilityEntry const* ability = sBattlePetAbilityStore.LookupEntry(abilityID);
            DamageResult dmg = CalculateAbilityDamage(
                attacker.EffectivePower, attacker.EffectivePower,
                ability ? PetBattlePetType(ability->PetTypeEnum) : PetBattlePetType(attacker.PetType),
                attacker, defender);

            defender.Health = std::max(0, defender.Health - dmg.Damage);

            PetBattleRoundEffect effect;
            effect.AbilityEffectID = abilityID;
            effect.EffectType = PET_BATTLE_EFFECT_SET_HEALTH;
            effect.SourceTeam = attackerTeam;
            effect.SourcePet = attackerPet;
            effect.TargetTeam = defenderTeam;
            effect.TargetPet = defenderPet;
            effect.Param1 = defender.Health;
            if (dmg.IsCrit)
                effect.Flags |= PET_BATTLE_EFFECT_FLAG_CRIT;
            if (dmg.TypeMod > 1.0f)
                effect.Flags |= PET_BATTLE_EFFECT_FLAG_STRONG;
            else if (dmg.TypeMod < 1.0f)
                effect.Flags |= PET_BATTLE_EFFECT_FLAG_WEAK;
            _roundEffects.push_back(effect);

            ApplyPassiveOnDamageDealt(attackerTeam, attackerPet, defenderTeam, defenderPet, dmg.Damage);
        }
        return;
    }

    // Determine which turn in the sequence to execute
    uint8 turnIndex = 0;
    if (attacker.IsLockedByMultiTurn && attacker.MultiTurnAbilityID == int32(abilityID))
    {
        turnIndex = attacker.MultiTurnCurrentIndex;
    }

    // Check if this is a multi-turn ability
    if (turns->size() > 1)
    {
        if (!attacker.IsLockedByMultiTurn)
        {
            // First turn of multi-turn: lock the pet
            attacker.MultiTurnAbilityID = abilityID;
            attacker.MultiTurnCurrentIndex = 0;
            attacker.MultiTurnTotalTurns = static_cast<int8>(turns->size());
            attacker.IsLockedByMultiTurn = true;
            turnIndex = 0;

            // Set lockdown on abilities during multi-turn
            for (uint8 a = 0; a < MAX_PET_BATTLE_ABILITIES; ++a)
                if (attacker.AbilityIDs[a] != 0)
                    attacker.AbilityLockdowns[a] = attacker.MultiTurnTotalTurns;
        }
        else
        {
            // Advance to next turn
            attacker.MultiTurnCurrentIndex++;
            turnIndex = attacker.MultiTurnCurrentIndex;

            // Check if multi-turn sequence is complete
            if (attacker.MultiTurnCurrentIndex >= attacker.MultiTurnTotalTurns)
            {
                attacker.MultiTurnAbilityID = 0;
                attacker.MultiTurnCurrentIndex = 0;
                attacker.MultiTurnTotalTurns = 0;
                attacker.IsLockedByMultiTurn = false;
            }
        }
    }

    if (turnIndex >= turns->size())
        return;

    BattlePetAbilityTurnEntry const* turn = (*turns)[turnIndex];

    // Get effects for this turn from DB2 index
    std::vector<BattlePetAbilityEffectEntry const*> const* effects = sPetBattleMgr->GetTurnEffectsFull(turn->ID);
    if (!effects)
        return;

    // Process each effect in order
    for (BattlePetAbilityEffectEntry const* effectEntry : *effects)
    {
        ProcessEffect(effectEntry, attackerTeam, attackerPet, defenderTeam, defenderPet, abilityID);
    }
}

void PetBattle::ProcessEffect(BattlePetAbilityEffectEntry const* effect, uint8 attackerTeam, uint8 attackerPet,
    uint8 defenderTeam, uint8 defenderPet, uint32 abilityID)
{
    if (!effect)
        return;

    PetBattlePetData& attacker = _teams[attackerTeam].Pets[attackerPet];
    PetBattlePetData& defender = _teams[defenderTeam].Pets[defenderPet];

    if (!attacker.IsAlive())
        return;

    BattlePetAbilityEntry const* ability = sBattlePetAbilityStore.LookupEntry(abilityID);
    PetBattlePetType abilityType = ability ? PetBattlePetType(ability->PetTypeEnum) : PetBattlePetType(attacker.PetType);

    // Determine effect action from BattlePetEffectPropertiesEntry
    uint16 effectPropsID = effect->BattlePetEffectPropertiesID;

    // The Param array from BattlePetAbilityEffectEntry provides effect-specific values
    // Param[0] typically = base power/amount, Param[1] = accuracy%, Param[2-5] = additional modifiers
    int16 basePower = effect->Param[0];
    int16 accuracy = effect->Param[1];

    // Check accuracy (if specified and > 0, roll for hit)
    if (accuracy > 0 && accuracy < 100)
    {
        if (urand(0, 99) >= uint32(accuracy))
        {
            // Create miss effect so client shows the miss indicator
            PetBattleRoundEffect missEffect;
            missEffect.AbilityEffectID = effect->ID;
            missEffect.EffectType = PET_BATTLE_EFFECT_SET_HEALTH;
            missEffect.Flags = PET_BATTLE_EFFECT_FLAG_MISS;
            missEffect.SourceTeam = attackerTeam;
            missEffect.SourcePet = attackerPet;
            missEffect.TargetTeam = defenderTeam;
            missEffect.TargetPet = defenderPet;
            missEffect.Param1 = defender.Health; // Health unchanged
            _roundEffects.push_back(missEffect);
            return;
        }
    }

    // Route to appropriate handler based on effect properties ID
    // We map the DB2 BattlePetEffectPropertiesID to our action types
    // In practice, the ID maps to specific effect behaviors
    BattlePetEffectPropertiesEntry const* effectProps = sBattlePetEffectPropertiesStore.LookupEntry(effectPropsID);
    if (!effectProps)
    {
        // If no properties entry, treat as simple damage with basePower
        if (basePower > 0 && defender.IsAlive())
        {
            DamageResult dmg = CalculateAbilityDamage(basePower, attacker.EffectivePower, abilityType, attacker, defender);
            defender.Health = std::max(0, defender.Health - dmg.Damage);

            PetBattleRoundEffect roundEffect;
            roundEffect.AbilityEffectID = effect->ID;
            roundEffect.EffectType = PET_BATTLE_EFFECT_SET_HEALTH;
            roundEffect.SourceTeam = attackerTeam;
            roundEffect.SourcePet = attackerPet;
            roundEffect.TargetTeam = defenderTeam;
            roundEffect.TargetPet = defenderPet;
            roundEffect.Param1 = defender.Health;
            if (dmg.IsCrit)
                roundEffect.Flags |= PET_BATTLE_EFFECT_FLAG_CRIT;
            if (dmg.TypeMod > 1.0f)
                roundEffect.Flags |= PET_BATTLE_EFFECT_FLAG_STRONG;
            else if (dmg.TypeMod < 1.0f)
                roundEffect.Flags |= PET_BATTLE_EFFECT_FLAG_WEAK;
            _roundEffects.push_back(roundEffect);

            ApplyPassiveOnDamageDealt(attackerTeam, attackerPet, defenderTeam, defenderPet, dmg.Damage);
        }
        return;
    }

    // Use ParamTypeEnum to determine the effect type behavior
    // ParamTypeEnum[0] encodes the general category of the effect
    uint8 effectCategory = effectProps->ParamTypeEnum[0];

    // Map effect categories to actions
    // effectCategory values are stable across client versions
    switch (effectCategory)
    {
        case 0: // Direct damage
        case 14: // Capped damage
        {
            if (!defender.IsAlive())
                break;

            DamageResult dmg = CalculateAbilityDamage(basePower, attacker.EffectivePower, abilityType, attacker, defender);
            defender.Health = std::max(0, defender.Health - dmg.Damage);

            PetBattleRoundEffect roundEffect;
            roundEffect.AbilityEffectID = effect->ID;
            roundEffect.EffectType = PET_BATTLE_EFFECT_SET_HEALTH;
            roundEffect.SourceTeam = attackerTeam;
            roundEffect.SourcePet = attackerPet;
            roundEffect.TargetTeam = defenderTeam;
            roundEffect.TargetPet = defenderPet;
            roundEffect.Param1 = defender.Health;
            if (dmg.IsCrit)
                roundEffect.Flags |= PET_BATTLE_EFFECT_FLAG_CRIT;
            if (dmg.TypeMod > 1.0f)
                roundEffect.Flags |= PET_BATTLE_EFFECT_FLAG_STRONG;
            else if (dmg.TypeMod < 1.0f)
                roundEffect.Flags |= PET_BATTLE_EFFECT_FLAG_WEAK;
            _roundEffects.push_back(roundEffect);

            ApplyPassiveOnDamageDealt(attackerTeam, attackerPet, defenderTeam, defenderPet, dmg.Damage);
            break;
        }
        case 1: // Healing
        case 15: // Capped healing
        {
            int32 healing = CalculateAbilityHealing(basePower, attacker.EffectivePower, attacker);
            attacker.Health = std::min(attacker.MaxHealth, attacker.Health + healing);

            PetBattleRoundEffect roundEffect;
            roundEffect.AbilityEffectID = effect->ID;
            roundEffect.EffectType = PET_BATTLE_EFFECT_SET_HEALTH;
            roundEffect.Flags = PET_BATTLE_EFFECT_FLAG_HEAL;
            roundEffect.SourceTeam = attackerTeam;
            roundEffect.SourcePet = attackerPet;
            roundEffect.TargetTeam = attackerTeam;
            roundEffect.TargetPet = attackerPet;
            roundEffect.Param1 = attacker.Health;
            _roundEffects.push_back(roundEffect);
            break;
        }
        case 2: // Apply aura (DoT/HoT/Buff/Debuff)
        case 12: // Periodic damage aura
        {
            int8 auraDuration = static_cast<int8>(effect->Param[2]);
            if (auraDuration <= 0)
                auraDuration = 3; // Default duration

            DamageResult dmg = CalculateAbilityDamage(basePower, attacker.EffectivePower, abilityType, attacker, defender);
            int32 tickDamage = dmg.Damage;

            PetBattleAuraType auraType = (effectCategory == 12) ? PET_BATTLE_AURA_DOT : PET_BATTLE_AURA_DEBUFF;
            if (basePower < 0) // Negative power = healing aura
                auraType = PET_BATTLE_AURA_HOT;

            // Critter passive: immune to stun, root, sleep
            if (defender.PetType == PET_TYPE_CRITTER &&
                (auraType == PET_BATTLE_AURA_STUN || auraType == PET_BATTLE_AURA_ROOT || auraType == PET_BATTLE_AURA_SLEEP))
            {
                PetBattleRoundEffect immuneEffect;
                immuneEffect.AbilityEffectID = effect->ID;
                immuneEffect.EffectType = PET_BATTLE_EFFECT_SET_STATE;
                immuneEffect.Flags = PET_BATTLE_EFFECT_FLAG_IMMUNE;
                immuneEffect.SourceTeam = attackerTeam;
                immuneEffect.SourcePet = attackerPet;
                immuneEffect.TargetTeam = defenderTeam;
                immuneEffect.TargetPet = defenderPet;
                _roundEffects.push_back(immuneEffect);
                break;
            }

            AddAura(defenderTeam, defenderPet, abilityID, effect->ID,
                auraType, auraDuration, tickDamage, attacker.PetType,
                attackerTeam, attackerPet);

            PetBattleRoundEffect roundEffect;
            roundEffect.AbilityEffectID = effect->ID;
            roundEffect.EffectType = PET_BATTLE_EFFECT_AURA_APPLY;
            roundEffect.SourceTeam = attackerTeam;
            roundEffect.SourcePet = attackerPet;
            roundEffect.TargetTeam = defenderTeam;
            roundEffect.TargetPet = defenderPet;
            roundEffect.Param1 = abilityID;
            roundEffect.Param2 = auraDuration;
            _roundEffects.push_back(roundEffect);
            break;
        }
        case 13: // Periodic heal aura
        {
            int8 auraDuration = static_cast<int8>(effect->Param[2]);
            if (auraDuration <= 0)
                auraDuration = 3;

            int32 tickHealing = CalculateAbilityHealing(basePower, attacker.EffectivePower, attacker);

            AddAura(attackerTeam, attackerPet, abilityID, effect->ID,
                PET_BATTLE_AURA_HOT, auraDuration, tickHealing, attacker.PetType,
                attackerTeam, attackerPet);

            PetBattleRoundEffect roundEffect;
            roundEffect.AbilityEffectID = effect->ID;
            roundEffect.EffectType = PET_BATTLE_EFFECT_AURA_APPLY;
            roundEffect.SourceTeam = attackerTeam;
            roundEffect.SourcePet = attackerPet;
            roundEffect.TargetTeam = attackerTeam;
            roundEffect.TargetPet = attackerPet;
            roundEffect.Param1 = abilityID;
            roundEffect.Param2 = auraDuration;
            _roundEffects.push_back(roundEffect);
            break;
        }
        case 3: // Change state
        case 6: // Set state
        {
            uint32 stateID = static_cast<uint32>(effect->Param[2]);
            int32 stateValue = static_cast<int32>(effect->Param[3]);

            bool found = false;
            for (auto& [id, val] : defender.States)
            {
                if (id == stateID)
                {
                    val = (effectCategory == 3) ? val + stateValue : stateValue;
                    found = true;
                    break;
                }
            }
            if (!found)
                defender.States.emplace_back(stateID, stateValue);

            PetBattleRoundEffect roundEffect;
            roundEffect.AbilityEffectID = effect->ID;
            roundEffect.EffectType = PET_BATTLE_EFFECT_SET_STATE;
            roundEffect.SourceTeam = attackerTeam;
            roundEffect.SourcePet = attackerPet;
            roundEffect.TargetTeam = defenderTeam;
            roundEffect.TargetPet = defenderPet;
            roundEffect.Param1 = stateID;
            roundEffect.Param2 = stateValue;
            _roundEffects.push_back(roundEffect);
            break;
        }
        case 4: // Damage percentage
        {
            if (!defender.IsAlive())
                break;

            int32 damage = int32(defender.MaxHealth * (basePower / 100.0f));
            if (damage < 1) damage = 1;

            // Apply type effectiveness
            float typeMod = GetTypeEffectiveness(abilityType, PetBattlePetType(defender.PetType));
            damage = int32(damage * typeMod);

            // Apply Magic passive: cannot take more than 35% max HP in a single hit
            if (defender.PetType == PET_TYPE_MAGIC)
                damage = std::min(damage, int32(defender.MaxHealth * PASSIVE_MAGIC_DAMAGE_CAP_PCT));

            if (damage < 1) damage = 1;
            defender.Health = std::max(0, defender.Health - damage);

            PetBattleRoundEffect roundEffect;
            roundEffect.AbilityEffectID = effect->ID;
            roundEffect.EffectType = PET_BATTLE_EFFECT_SET_HEALTH;
            roundEffect.SourceTeam = attackerTeam;
            roundEffect.SourcePet = attackerPet;
            roundEffect.TargetTeam = defenderTeam;
            roundEffect.TargetPet = defenderPet;
            roundEffect.Param1 = defender.Health;
            _roundEffects.push_back(roundEffect);

            ApplyPassiveOnDamageDealt(attackerTeam, attackerPet, defenderTeam, defenderPet, damage);
            break;
        }
        case 5: // Heal percentage
        {
            int32 healing = int32(attacker.MaxHealth * (basePower / 100.0f));
            attacker.Health = std::min(attacker.MaxHealth, attacker.Health + healing);

            PetBattleRoundEffect roundEffect;
            roundEffect.AbilityEffectID = effect->ID;
            roundEffect.EffectType = PET_BATTLE_EFFECT_SET_HEALTH;
            roundEffect.Flags = PET_BATTLE_EFFECT_FLAG_HEAL;
            roundEffect.SourceTeam = attackerTeam;
            roundEffect.SourcePet = attackerPet;
            roundEffect.TargetTeam = attackerTeam;
            roundEffect.TargetPet = attackerPet;
            roundEffect.Param1 = attacker.Health;
            _roundEffects.push_back(roundEffect);
            break;
        }
        case 7: // Force pet swap
        {
            if (defender.IsAlive() && _teams[defenderTeam].PetCount > 1)
            {
                // Swap to a random different alive pet
                for (uint8 i = 0; i < _teams[defenderTeam].PetCount; ++i)
                {
                    if (i != _teams[defenderTeam].FrontPetIndex &&
                        _teams[defenderTeam].Pets[i].IsAlive() &&
                        !_teams[defenderTeam].Pets[i].IsCaptured)
                    {
                        _teams[defenderTeam].FrontPetIndex = static_cast<int8>(i);

                        PetBattleRoundEffect roundEffect;
                        roundEffect.AbilityEffectID = effect->ID;
                        roundEffect.EffectType = PET_BATTLE_EFFECT_PET_SWAP;
                        roundEffect.SourceTeam = attackerTeam;
                        roundEffect.SourcePet = attackerPet;
                        roundEffect.TargetTeam = defenderTeam;
                        roundEffect.TargetPet = i;
                        _roundEffects.push_back(roundEffect);
                        break;
                    }
                }
            }
            break;
        }
        case 9: // Change max health
        {
            int32 healthChange = static_cast<int32>(basePower);
            defender.MaxHealth = std::max(1, defender.MaxHealth + healthChange);
            defender.Health = std::min(defender.Health, defender.MaxHealth);

            PetBattleRoundEffect roundEffect;
            roundEffect.AbilityEffectID = effect->ID;
            roundEffect.EffectType = PET_BATTLE_EFFECT_SET_MAX_HEALTH;
            roundEffect.SourceTeam = attackerTeam;
            roundEffect.SourcePet = attackerPet;
            roundEffect.TargetTeam = defenderTeam;
            roundEffect.TargetPet = defenderPet;
            roundEffect.Param1 = defender.MaxHealth;
            _roundEffects.push_back(roundEffect);
            break;
        }
        case 10: // Set weather
        {
            PetBattleWeatherType weatherType = static_cast<PetBattleWeatherType>(effect->Param[2]);
            int8 weatherDuration = static_cast<int8>(effect->Param[3]);
            if (weatherDuration <= 0)
                weatherDuration = 9; // Default weather duration

            SetWeather(weatherType, abilityID, weatherDuration, attackerTeam);
            break;
        }
        case 11: // Stun
        {
            // Critter passive: immune to stun
            if (defender.PetType == PET_TYPE_CRITTER)
            {
                PetBattleRoundEffect immuneEffect;
                immuneEffect.AbilityEffectID = effect->ID;
                immuneEffect.EffectType = PET_BATTLE_EFFECT_SET_STATE;
                immuneEffect.Flags = PET_BATTLE_EFFECT_FLAG_IMMUNE;
                immuneEffect.SourceTeam = attackerTeam;
                immuneEffect.SourcePet = attackerPet;
                immuneEffect.TargetTeam = defenderTeam;
                immuneEffect.TargetPet = defenderPet;
                _roundEffects.push_back(immuneEffect);
                break;
            }

            defender.IsStunned = true;

            PetBattleRoundEffect roundEffect;
            roundEffect.AbilityEffectID = effect->ID;
            roundEffect.EffectType = PET_BATTLE_EFFECT_SET_STATE;
            roundEffect.SourceTeam = attackerTeam;
            roundEffect.SourcePet = attackerPet;
            roundEffect.TargetTeam = defenderTeam;
            roundEffect.TargetPet = defenderPet;
            roundEffect.Param1 = 1; // stunned
            _roundEffects.push_back(roundEffect);
            break;
        }
        case 16: // Remove aura
        {
            uint32 auraAbilityID = static_cast<uint32>(effect->Param[2]);
            if (auraAbilityID == 0)
                auraAbilityID = abilityID;
            RemoveAura(defenderTeam, defenderPet, auraAbilityID);
            break;
        }
        default:
            // Unhandled effect category - treat as damage if basePower > 0
            if (basePower > 0 && defender.IsAlive())
            {
                DamageResult dmg = CalculateAbilityDamage(basePower, attacker.EffectivePower, abilityType, attacker, defender);
                defender.Health = std::max(0, defender.Health - dmg.Damage);

                PetBattleRoundEffect roundEffect;
                roundEffect.AbilityEffectID = effect->ID;
                roundEffect.EffectType = PET_BATTLE_EFFECT_SET_HEALTH;
                roundEffect.SourceTeam = attackerTeam;
                roundEffect.SourcePet = attackerPet;
                roundEffect.TargetTeam = defenderTeam;
                roundEffect.TargetPet = defenderPet;
                roundEffect.Param1 = defender.Health;
                if (dmg.IsCrit)
                    roundEffect.Flags |= PET_BATTLE_EFFECT_FLAG_CRIT;
                if (dmg.TypeMod > 1.0f)
                    roundEffect.Flags |= PET_BATTLE_EFFECT_FLAG_STRONG;
                else if (dmg.TypeMod < 1.0f)
                    roundEffect.Flags |= PET_BATTLE_EFFECT_FLAG_WEAK;
                _roundEffects.push_back(roundEffect);

                ApplyPassiveOnDamageDealt(attackerTeam, attackerPet, defenderTeam, defenderPet, dmg.Damage);
            }
            break;
    }
}

// ============================================================================
// Damage / Healing calculations with passive abilities
// ============================================================================

DamageResult PetBattle::CalculateAbilityDamage(int32 abilityPower, int32 attackerPower, PetBattlePetType abilityType,
    PetBattlePetData const& attacker, PetBattlePetData& defender)
{
    DamageResult result;

    // Formula: rawDamage = abilityPower * (attackerPower / 20.0f)
    float rawDamage = abilityPower * (attackerPower / 20.0f);

    // Type effectiveness modifier
    result.TypeMod = GetTypeEffectiveness(abilityType, PetBattlePetType(defender.PetType));
    rawDamage *= result.TypeMod;

    // Weather damage modifier
    rawDamage *= (1.0f + GetWeatherDamageModifier(abilityType));

    // Beast passive: +25% damage when below 50% HP
    if (attacker.PetType == PET_TYPE_BEAST && attacker.Health <= attacker.MaxHealth / 2)
        rawDamage *= (1.0f + PASSIVE_BEAST_DAMAGE_BONUS);

    // Dragonkin passive: +50% damage on round after bringing enemy below 50%
    if (attacker.DragonkinDamageBonus)
        rawDamage *= (1.0f + PASSIVE_DRAGONKIN_DAMAGE_BONUS);

    // Critical hit check (5% base chance, 1.5x multiplier)
    if (frand(0.0f, 1.0f) < PET_BATTLE_BASE_CRIT_CHANCE)
    {
        rawDamage *= PET_BATTLE_CRIT_MULTIPLIER;
        result.IsCrit = true;
    }

    int32 damage = std::max(1, int32(std::round(rawDamage)));

    // Magic passive: cannot take more than 35% max HP in a single hit
    if (defender.PetType == PET_TYPE_MAGIC)
        damage = std::min(damage, int32(defender.MaxHealth * PASSIVE_MAGIC_DAMAGE_CAP_PCT));

    // Boss pets take reduced damage (capped at ~35% max HP per hit)
    if (BattlePetSpeciesEntry const* defenderSpecies = sBattlePetSpeciesStore.LookupEntry(defender.Species))
        if (defenderSpecies->GetFlags().HasFlag(BattlePetSpeciesFlags::Boss))
            damage = std::min(damage, int32(defender.MaxHealth * 0.35f));

    result.Damage = std::max(1, damage);
    return result;
}

int32 PetBattle::CalculateAbilityHealing(int32 healPower, int32 attackerPower, PetBattlePetData const& healer)
{
    float rawHealing = healPower * (attackerPower / 20.0f);

    // Weather healing modifier
    rawHealing *= (1.0f + GetWeatherHealingModifier());

    return std::max(1, int32(std::round(rawHealing)));
}

// ============================================================================
// Aura system
// ============================================================================

void PetBattle::AddAura(uint8 targetTeam, uint8 targetPet, uint32 abilityID, uint32 effectID,
    PetBattleAuraType auraType, int32 duration, int32 damagePerTick, int8 petType,
    uint8 casterTeam, uint8 casterPet)
{
    PetBattlePetData& pet = _teams[targetTeam].Pets[targetPet];

    // Max aura limit
    if (pet.Auras.size() >= MAX_PET_BATTLE_AURAS)
    {
        // Remove oldest aura to make room
        if (!pet.Auras.empty())
        {
            PetBattleRoundEffect removeEffect;
            removeEffect.EffectType = PET_BATTLE_EFFECT_AURA_CANCEL;
            removeEffect.TargetTeam = targetTeam;
            removeEffect.TargetPet = targetPet;
            removeEffect.Param1 = pet.Auras.front().AbilityID;
            _roundEffects.push_back(removeEffect);

            pet.Auras.erase(pet.Auras.begin());
        }
    }

    PetBattleAura aura;
    aura.AbilityID = abilityID;
    aura.EffectID = effectID;
    aura.Duration = duration;
    aura.RemainingRounds = duration;
    aura.AuraInstanceID = _nextAuraInstanceID++;
    aura.CurrentRound = _currentRound;
    aura.CasterTeam = casterTeam;
    aura.CasterPet = casterPet;
    aura.AuraType = auraType;
    aura.DamagePerTick = damagePerTick;
    aura.PetType = petType;
    aura.StateFlags = PET_BATTLE_AURA_STATE_JUST_APPLIED;
    if (duration <= 0)
        aura.StateFlags |= PET_BATTLE_AURA_STATE_INFINITE;

    pet.Auras.push_back(aura);
}

void PetBattle::RemoveAura(uint8 targetTeam, uint8 targetPet, uint32 abilityID)
{
    PetBattlePetData& pet = _teams[targetTeam].Pets[targetPet];

    auto it = std::find_if(pet.Auras.begin(), pet.Auras.end(),
        [abilityID](PetBattleAura const& aura) { return aura.AbilityID == abilityID; });

    if (it != pet.Auras.end())
    {
        PetBattleRoundEffect roundEffect;
        roundEffect.EffectType = PET_BATTLE_EFFECT_AURA_CANCEL;
        roundEffect.TargetTeam = targetTeam;
        roundEffect.TargetPet = targetPet;
        roundEffect.Param1 = abilityID;
        _roundEffects.push_back(roundEffect);

        pet.Auras.erase(it);
    }
}

void PetBattle::TickAuras()
{
    for (uint8 t = 0; t < MAX_PET_BATTLE_PLAYERS; ++t)
    {
        for (uint8 p = 0; p < _teams[t].PetCount; ++p)
        {
            PetBattlePetData& pet = _teams[t].Pets[p];
            if (!pet.IsAlive())
                continue;

            // Clear JUST_APPLIED flag from previous round
            for (PetBattleAura& aura : pet.Auras)
                aura.StateFlags &= ~PET_BATTLE_AURA_STATE_JUST_APPLIED;

            // Process auras in reverse order for safe removal
            for (int32 i = static_cast<int32>(pet.Auras.size()) - 1; i >= 0; --i)
            {
                PetBattleAura& aura = pet.Auras[i];

                // Tick periodic effects
                if (aura.AuraType == PET_BATTLE_AURA_DOT && aura.DamagePerTick > 0)
                {
                    int32 tickDamage = aura.DamagePerTick;

                    // Apply type effectiveness for DoTs
                    if (aura.PetType >= 0 && aura.PetType < PET_TYPE_COUNT)
                    {
                        float typeMod = GetTypeEffectiveness(PetBattlePetType(aura.PetType), PetBattlePetType(pet.PetType));
                        tickDamage = int32(tickDamage * typeMod);
                    }

                    // Aquatic passive: DoTs deal 25% less damage
                    if (pet.PetType == PET_TYPE_AQUATIC)
                        tickDamage = int32(tickDamage * (1.0f - PASSIVE_AQUATIC_DOT_REDUCTION));

                    if (tickDamage < 1) tickDamage = 1;
                    pet.Health = std::max(0, pet.Health - tickDamage);

                    PetBattleRoundEffect roundEffect;
                    roundEffect.AbilityEffectID = aura.EffectID;
                    roundEffect.EffectType = PET_BATTLE_EFFECT_SET_HEALTH;
                    roundEffect.SourceTeam = aura.CasterTeam;
                    roundEffect.SourcePet = aura.CasterPet;
                    roundEffect.TargetTeam = t;
                    roundEffect.TargetPet = p;
                    roundEffect.Param1 = pet.Health;
                    _roundEffects.push_back(roundEffect);
                }
                else if (aura.AuraType == PET_BATTLE_AURA_HOT && aura.DamagePerTick > 0)
                {
                    int32 tickHealing = aura.DamagePerTick;
                    pet.Health = std::min(pet.MaxHealth, pet.Health + tickHealing);

                    PetBattleRoundEffect roundEffect;
                    roundEffect.AbilityEffectID = aura.EffectID;
                    roundEffect.EffectType = PET_BATTLE_EFFECT_SET_HEALTH;
                    roundEffect.SourceTeam = aura.CasterTeam;
                    roundEffect.SourcePet = aura.CasterPet;
                    roundEffect.TargetTeam = t;
                    roundEffect.TargetPet = p;
                    roundEffect.Param1 = pet.Health;
                    _roundEffects.push_back(roundEffect);
                }

                // Decrement remaining rounds
                aura.RemainingRounds--;

                // Remove expired auras
                if (aura.RemainingRounds <= 0)
                {
                    PetBattleRoundEffect roundEffect;
                    roundEffect.EffectType = PET_BATTLE_EFFECT_AURA_CANCEL;
                    roundEffect.TargetTeam = t;
                    roundEffect.TargetPet = p;
                    roundEffect.Param1 = aura.AbilityID;
                    _roundEffects.push_back(roundEffect);

                    pet.Auras.erase(pet.Auras.begin() + i);
                }
            }
        }
    }
}

// ============================================================================
// Environment / Weather system
// ============================================================================

void PetBattle::SetWeather(PetBattleWeatherType weatherType, uint32 abilityID, int8 duration, uint8 casterTeam)
{
    if (weatherType == PET_BATTLE_WEATHER_CLEANSING)
    {
        // Clear all weather
        for (auto& env : _environments)
        {
            env.WeatherType = PET_BATTLE_WEATHER_NONE;
            env.AbilityID = 0;
            env.RemainingRounds = 0;
        }
        return;
    }

    // Slot 0 is the shared battlefield weather
    _environments[0].AbilityID = abilityID;
    _environments[0].WeatherType = weatherType;
    _environments[0].RemainingRounds = duration;
    _environments[0].CasterTeam = casterTeam;
}

void PetBattle::TickWeather()
{
    for (auto& env : _environments)
    {
        if (env.WeatherType == PET_BATTLE_WEATHER_NONE)
            continue;

        env.RemainingRounds--;

        // Weather deals periodic damage/effects
        if (env.WeatherType == PET_BATTLE_WEATHER_SANDSTORM ||
            env.WeatherType == PET_BATTLE_WEATHER_BLIZZARD ||
            env.WeatherType == PET_BATTLE_WEATHER_LIGHTNING)
        {
            for (uint8 t = 0; t < MAX_PET_BATTLE_PLAYERS; ++t)
            {
                PetBattlePetData& frontPet = _teams[t].Pets[_teams[t].FrontPetIndex];
                if (!frontPet.IsAlive())
                    continue;

                // Elemental passive: ignores all weather effects
                if (frontPet.PetType == PET_TYPE_ELEMENTAL)
                    continue;

                int32 weatherDamage = std::max(1, frontPet.MaxHealth / 20); // 5% max HP per tick

                frontPet.Health = std::max(0, frontPet.Health - weatherDamage);

                PetBattleRoundEffect roundEffect;
                roundEffect.AbilityEffectID = env.AbilityID;
                roundEffect.EffectType = PET_BATTLE_EFFECT_SET_HEALTH;
                roundEffect.TargetTeam = t;
                roundEffect.TargetPet = _teams[t].FrontPetIndex;
                roundEffect.Param1 = frontPet.Health;
                _roundEffects.push_back(roundEffect);
            }
        }

        if (env.RemainingRounds <= 0)
        {
            env.WeatherType = PET_BATTLE_WEATHER_NONE;
            env.AbilityID = 0;
        }
    }
}

float PetBattle::GetWeatherDamageModifier(PetBattlePetType abilityType) const
{
    PetBattleWeatherType weather = _environments[0].WeatherType;

    switch (weather)
    {
        case PET_BATTLE_WEATHER_RAIN:
            return (abilityType == PET_TYPE_AQUATIC) ? 0.25f : 0.0f;
        case PET_BATTLE_WEATHER_ARCANE:
        case PET_BATTLE_WEATHER_MOONLIGHT:
            return (abilityType == PET_TYPE_MAGIC) ? 0.25f : 0.0f;
        case PET_BATTLE_WEATHER_LIGHTNING:
            return (abilityType == PET_TYPE_MECHANICAL) ? 0.25f : 0.0f;
        case PET_BATTLE_WEATHER_SCORCHED:
            return (abilityType == PET_TYPE_DRAGONKIN) ? 0.25f : 0.0f;
        case PET_BATTLE_WEATHER_SANDSTORM:
            return -0.10f; // Sandstorm reduces all damage slightly
        default:
            return 0.0f;
    }
}

float PetBattle::GetWeatherHealingModifier() const
{
    PetBattleWeatherType weather = _environments[0].WeatherType;

    switch (weather)
    {
        case PET_BATTLE_WEATHER_SUNNY:
        case PET_BATTLE_WEATHER_MOONLIGHT:
            return 0.25f;
        default:
            return 0.0f;
    }
}

// ============================================================================
// Passive family abilities
// ============================================================================

void PetBattle::ApplyPassiveRoundStart()
{
    for (uint8 t = 0; t < MAX_PET_BATTLE_PLAYERS; ++t)
    {
        PetBattlePetData& frontPet = _teams[t].Pets[_teams[t].FrontPetIndex];
        if (!frontPet.IsAlive())
            continue;

        // Humanoid: Recovers 4% max HP each round
        if (frontPet.PetType == PET_TYPE_HUMANOID)
        {
            int32 healAmount = int32(frontPet.MaxHealth * PASSIVE_HUMANOID_HEAL_PCT);
            if (healAmount > 0 && frontPet.Health < frontPet.MaxHealth)
            {
                frontPet.Health = std::min(frontPet.MaxHealth, frontPet.Health + healAmount);

                PetBattleRoundEffect roundEffect;
                roundEffect.EffectType = PET_BATTLE_EFFECT_SET_HEALTH;
                roundEffect.TargetTeam = t;
                roundEffect.TargetPet = _teams[t].FrontPetIndex;
                roundEffect.Param1 = frontPet.Health;
                _roundEffects.push_back(roundEffect);
            }
        }

        // Dragonkin: reset damage bonus flag at start of round
        // (it was set by ApplyPassiveOnDamageDealt in the previous round)
        // The flag persists from previous round - it's checked in CalculateAbilityDamage
    }
}

void PetBattle::ApplyPassiveOnDamageDealt(uint8 attackerTeam, uint8 attackerPet,
    uint8 defenderTeam, uint8 defenderPet, int32 damage)
{
    PetBattlePetData& attacker = _teams[attackerTeam].Pets[attackerPet];
    PetBattlePetData& defender = _teams[defenderTeam].Pets[defenderPet];

    // Dragonkin: +50% damage next round after bringing enemy below 50% HP
    if (attacker.PetType == PET_TYPE_DRAGONKIN)
    {
        int32 previousHealth = defender.Health + damage; // Health before this hit
        if (previousHealth > defender.MaxHealth / 2 && defender.Health <= defender.MaxHealth / 2)
            attacker.DragonkinDamageBonus = true;
    }
}

bool PetBattle::ApplyPassiveOnDeath(uint8 teamIdx, uint8 petIdx)
{
    PetBattlePetData& pet = _teams[teamIdx].Pets[petIdx];

    // Undead: Returns to life for one round when killed (once per battle)
    if (pet.PetType == PET_TYPE_UNDEAD && !pet.UndeadReviveUsed)
    {
        pet.UndeadReviveUsed = true;
        pet.IsUndeadReviving = true;
        pet.Health = pet.MaxHealth; // Full HP for the revive round

        PetBattleRoundEffect roundEffect;
        roundEffect.EffectType = PET_BATTLE_EFFECT_SET_HEALTH;
        roundEffect.TargetTeam = teamIdx;
        roundEffect.TargetPet = petIdx;
        roundEffect.Param1 = pet.Health;
        _roundEffects.push_back(roundEffect);
        return true;
    }

    // Mechanical: Comes back to life once per battle at 20% HP
    if (pet.PetType == PET_TYPE_MECHANICAL && !pet.MechanicalReviveUsed)
    {
        pet.MechanicalReviveUsed = true;
        pet.Health = int32(pet.MaxHealth * PASSIVE_MECHANICAL_REVIVE_PCT);
        if (pet.Health < 1) pet.Health = 1;

        PetBattleRoundEffect roundEffect;
        roundEffect.EffectType = PET_BATTLE_EFFECT_SET_HEALTH;
        roundEffect.TargetTeam = teamIdx;
        roundEffect.TargetPet = petIdx;
        roundEffect.Param1 = pet.Health;
        _roundEffects.push_back(roundEffect);
        return true;
    }

    return false;
}

// ============================================================================
// Speed resolution
// ============================================================================

void PetBattle::ResolveSpeed()
{
    // Speed already factored into ProcessRound() turn ordering
    // This method is kept for compatibility
}

// ============================================================================
// Death checking
// ============================================================================

void PetBattle::CheckDeaths()
{
    for (uint8 t = 0; t < MAX_PET_BATTLE_PLAYERS; ++t)
    {
        for (uint8 p = 0; p < _teams[t].PetCount; ++p)
        {
            PetBattlePetData& pet = _teams[t].Pets[p];
            if (pet.Health <= 0 && !pet.IsCaptured && !_petKilledThisRound[t * MAX_PET_BATTLE_TEAM_SIZE + p])
            {
                // Try passive resurrection (Undead / Mechanical)
                if (ApplyPassiveOnDeath(t, p))
                    continue;

                _petKilledThisRound[t * MAX_PET_BATTLE_TEAM_SIZE + p] = true;
            }
        }
    }
}

// ============================================================================
// XP calculation using GameTable
// ============================================================================

void PetBattle::AwardExperience()
{
    if (!_canAwardXP)
        return;

    if (_winnerTeam >= MAX_PET_BATTLE_PLAYERS)
        return;

    Player* player = GetPlayerForTeam(_winnerTeam);
    if (!player)
        return;

    PetBattleTeamData& winnerTeam = _teams[_winnerTeam];
    uint8 loserTeamIdx = _winnerTeam == PET_BATTLE_TEAM_1 ? PET_BATTLE_TEAM_2 : PET_BATTLE_TEAM_1;
    PetBattleTeamData& loserTeam = _teams[loserTeamIdx];

    // Find the highest level opponent for XP scaling
    uint16 maxOpponentLevel = 1;
    for (uint8 i = 0; i < loserTeam.PetCount; ++i)
        maxOpponentLevel = std::max(maxOpponentLevel, loserTeam.Pets[i].Level);

    // Award XP to all surviving pets on the winning team
    BattlePets::BattlePetMgr* petMgr = player->GetSession()->GetBattlePetMgr();

    for (uint8 i = 0; i < winnerTeam.PetCount; ++i)
    {
        PetBattlePetData& pet = winnerTeam.Pets[i];
        if (!pet.IsAlive() || pet.Level >= BattlePets::MAX_BATTLE_PET_LEVEL)
            continue;

        if (pet.BattlePetGUID.IsEmpty())
            continue;

        // Look up XP from GameTable
        GtBattlePetXPEntry const* xpEntry = sBattlePetXPGameTable.GetRow(maxOpponentLevel);
        uint16 xpAward = xpEntry ? static_cast<uint16>(GetBattlePetXPPerLevel(xpEntry)) : 100;

        // Scale by level difference: pets much lower than opponent get more XP
        int16 levelDiff = static_cast<int16>(maxOpponentLevel) - static_cast<int16>(pet.Level);
        if (levelDiff > 0)
            xpAward = static_cast<uint16>(xpAward * (1.0f + levelDiff * 0.1f)); // +10% per level below opponent
        else if (levelDiff < -5)
            xpAward = static_cast<uint16>(xpAward * 0.1f); // Heavily reduced for fighting much lower level

        if (xpAward < 1) xpAward = 1;

        petMgr->GrantBattlePetExperience(pet.BattlePetGUID, xpAward, BattlePets::BattlePetXpSource::PetBattle);
    }
}

// ============================================================================
// Battle end
// ============================================================================

void PetBattle::FinishBattle(PetBattleResult result)
{
    _state = PET_BATTLE_STATE_FINAL_ROUND;

    switch (result)
    {
        case PET_BATTLE_RESULT_TEAM_1_WIN:
            _winnerTeam = PET_BATTLE_TEAM_1;
            break;
        case PET_BATTLE_RESULT_TEAM_2_WIN:
            _winnerTeam = PET_BATTLE_TEAM_2;
            break;
        default:
            break;
    }

    AwardExperience();

    // Achievement criteria updates
    // Note: ModifierTreeType conditions (BattlePetFightWasPVP, BattlePetTeamLevel,
    // BattlePetTeamWithAliveEqualOrGreaterThan) are evaluated automatically by the
    // criteria system when checking criteria of these types.
    for (uint8 t = 0; t < MAX_PET_BATTLE_PLAYERS; ++t)
    {
        Player* player = GetPlayerForTeam(t);
        if (!player)
            continue;

        if (t == _winnerTeam)
            player->UpdateCriteria(CriteriaType::WinPetBattle);
        else
            player->UpdateCriteria(CriteriaType::LosePetBattle);
    }

    _state = PET_BATTLE_STATE_FINISHED;
}

void PetBattle::Forfeit(uint8 teamIdx)
{
    // Apply deserter penalty for PvP forfeits
    if (_battleType == PET_BATTLE_TYPE_PVP)
    {
        if (Player* player = GetPlayerForTeam(teamIdx))
        {
            // Pet Battle Deserter - 10 minute debuff (spell 150340)
            static constexpr uint32 SPELL_PET_BATTLE_DESERTER = 150340;
            player->CastSpell(player, SPELL_PET_BATTLE_DESERTER, true);
        }
    }

    if (teamIdx == PET_BATTLE_TEAM_1)
        FinishBattle(PET_BATTLE_RESULT_TEAM_2_WIN);
    else
        FinishBattle(PET_BATTLE_RESULT_TEAM_1_WIN);
}

// ============================================================================
// Wild / NPC AI
// ============================================================================

void PetBattle::GenerateWildTeamInput()
{
    PetBattleTeamData& wildTeam = _teams[PET_BATTLE_TEAM_2];
    PetBattlePetData& frontPet = wildTeam.Pets[wildTeam.FrontPetIndex];

    if (!frontPet.IsAlive())
    {
        // Swap to next alive pet
        int8 nextAlive = wildTeam.GetFirstAlivePetIndex();
        if (nextAlive >= 0 && nextAlive != wildTeam.FrontPetIndex)
        {
            SubmitInput(PET_BATTLE_TEAM_2, PET_BATTLE_MOVE_SWAP, 0, nextAlive);
            return;
        }
        // No alive pets, skip
        SubmitInput(PET_BATTLE_TEAM_2, PET_BATTLE_MOVE_PASS, 0, -1);
        return;
    }

    // If locked in multi-turn, skip input (will auto-continue)
    if (frontPet.IsLockedByMultiTurn)
    {
        SubmitInput(PET_BATTLE_TEAM_2, PET_BATTLE_MOVE_ABILITY, frontPet.MultiTurnAbilityID, -1);
        return;
    }

    // Collect available abilities (off cooldown, not zero)
    struct AbilityOption
    {
        uint32 abilityID;
        uint8 slotIdx;
        float priority;
    };
    std::vector<AbilityOption> availableAbilities;

    for (uint8 i = 0; i < MAX_PET_BATTLE_ABILITIES; ++i)
    {
        if (frontPet.AbilityIDs[i] == 0 || frontPet.AbilityCooldowns[i] > 0)
            continue;

        float priority = 1.0f;

        BattlePetAbilityEntry const* ability = sBattlePetAbilityStore.LookupEntry(frontPet.AbilityIDs[i]);
        if (ability)
        {
            // Prefer abilities with type advantage against defender
            PetBattlePetData const& defenderPet = _teams[PET_BATTLE_TEAM_1].Pets[_teams[PET_BATTLE_TEAM_1].FrontPetIndex];
            float typeMod = GetTypeEffectiveness(PetBattlePetType(ability->PetTypeEnum), PetBattlePetType(defenderPet.PetType));
            if (typeMod > 1.0f)
                priority += 2.0f; // Strong preference for super-effective
            else if (typeMod < 1.0f)
                priority -= 0.5f; // Slight avoidance of weak abilities
        }

        availableAbilities.push_back({ frontPet.AbilityIDs[i], i, priority });
    }

    if (availableAbilities.empty())
    {
        SubmitInput(PET_BATTLE_TEAM_2, PET_BATTLE_MOVE_PASS, 0, -1);
        return;
    }

    // Check if healing is desirable (HP < 50% and a healing ability exists)
    if (frontPet.Health < frontPet.MaxHealth / 2)
    {
        // For simplicity, we can't easily distinguish healing abilities from damage abilities
        // without deeper DB2 inspection. The type-effectiveness preference already provides
        // reasonable behavior.
    }

    // Consider swapping to a pet with type advantage if current pet is at disadvantage and low HP
    if (frontPet.Health < frontPet.MaxHealth / 3 && wildTeam.PetCount > 1)
    {
        PetBattlePetData const& defenderPet = _teams[PET_BATTLE_TEAM_1].Pets[_teams[PET_BATTLE_TEAM_1].FrontPetIndex];
        float currentTypeMod = GetTypeEffectiveness(PetBattlePetType(frontPet.PetType), PetBattlePetType(defenderPet.PetType));

        if (currentTypeMod < 1.0f) // Currently at a disadvantage
        {
            for (uint8 i = 0; i < wildTeam.PetCount; ++i)
            {
                if (i == wildTeam.FrontPetIndex || !wildTeam.Pets[i].IsAlive() || wildTeam.Pets[i].IsCaptured)
                    continue;

                float swapTypeMod = GetTypeEffectiveness(PetBattlePetType(wildTeam.Pets[i].PetType), PetBattlePetType(defenderPet.PetType));
                if (swapTypeMod > currentTypeMod && urand(0, 1)) // 50% chance to swap when advantageous
                {
                    SubmitInput(PET_BATTLE_TEAM_2, PET_BATTLE_MOVE_SWAP, 0, static_cast<int8>(i));
                    return;
                }
            }
        }
    }

    // Weighted random selection from available abilities
    float totalWeight = 0.0f;
    for (auto const& opt : availableAbilities)
        totalWeight += std::max(0.1f, opt.priority);

    float roll = frand(0.0f, totalWeight);
    float cumulative = 0.0f;

    for (auto const& opt : availableAbilities)
    {
        cumulative += std::max(0.1f, opt.priority);
        if (roll <= cumulative)
        {
            SubmitInput(PET_BATTLE_TEAM_2, PET_BATTLE_MOVE_ABILITY, opt.abilityID, -1);
            return;
        }
    }

    // Fallback: use first available ability
    SubmitInput(PET_BATTLE_TEAM_2, PET_BATTLE_MOVE_ABILITY, availableAbilities[0].abilityID, -1);
}

// ============================================================================
// Queries
// ============================================================================

bool PetBattle::WasPetKilledThisRound(uint8 teamIdx, uint8 petIdx) const
{
    return _petKilledThisRound[teamIdx * MAX_PET_BATTLE_TEAM_SIZE + petIdx];
}

bool PetBattle::NeedsFrontPetSwap(uint8 teamIdx) const
{
    return _needsFrontPetSwap[teamIdx];
}

Player* PetBattle::GetPlayerForTeam(uint8 teamIdx) const
{
    return ObjectAccessor::FindPlayer(_teams[teamIdx].PlayerGUID);
}

// ============================================================================
// Trap status validation
// ============================================================================

uint8 PetBattle::GetTrapStatus(uint8 playerTeam) const
{
    if (_battleType == PET_BATTLE_TYPE_PVP || _battleType == PET_BATTLE_TYPE_LFPB)
        return PET_BATTLE_TRAP_STATUS_CANT_TRAP_TRAINER_BATTLE;

    if (_battleType == PET_BATTLE_TYPE_NPC)
        return PET_BATTLE_TRAP_STATUS_CANT_TRAP_TRAINER_BATTLE;

    if (playerTeam != PET_BATTLE_TEAM_1)
        return PET_BATTLE_TRAP_STATUS_INVALID;

    PetBattleTeamData const& wildTeam = _teams[PET_BATTLE_TEAM_2];
    PetBattlePetData const& wildPet = wildTeam.Pets[wildTeam.FrontPetIndex];

    if (!wildPet.IsAlive())
        return PET_BATTLE_TRAP_STATUS_CANT_TRAP_PET_DEAD;

    if (wildPet.IsCaptured)
        return PET_BATTLE_TRAP_STATUS_CANT_TRAP_TWICE;

    // Check species is capturable (boss pets are never capturable)
    if (BattlePetSpeciesEntry const* species = sBattlePetSpeciesStore.LookupEntry(wildPet.Species))
    {
        if (!species->GetFlags().HasFlag(BattlePetSpeciesFlags::Capturable) ||
            species->GetFlags().HasFlag(BattlePetSpeciesFlags::Boss))
            return PET_BATTLE_TRAP_STATUS_CANT_TRAP_NOT_CAPTURABLE;
    }

    // Check health < 35% threshold
    float healthPct = wildPet.MaxHealth > 0 ? (float(wildPet.Health) / float(wildPet.MaxHealth)) : 1.0f;
    if (healthPct > 0.35f)
        return PET_BATTLE_TRAP_STATUS_CANT_TRAP_PET_HEALTH;

    // Check journal room
    Player* player = GetPlayerForTeam(playerTeam);
    if (player)
    {
        BattlePetSpeciesEntry const* species = sBattlePetSpeciesStore.LookupEntry(wildPet.Species);
        if (species && player->GetSession()->GetBattlePetMgr()->HasMaxPetCount(species, player->GetGUID()))
            return PET_BATTLE_TRAP_STATUS_CANT_TRAP_NO_ROOM;
    }

    return PET_BATTLE_TRAP_STATUS_CAN_TRAP;
}

// ============================================================================
// NPC Trainer Battle
// ============================================================================

void PetBattle::InitNPCBattle(Player* player, Creature* trainer, std::vector<NPCTeamPetInfo> const& npcTeam)
{
    _battleType = PET_BATTLE_TYPE_NPC;
    _canAwardXP = true;

    LoadPlayerTeam(player, _teams[PET_BATTLE_TEAM_1]);

    // Build NPC team from provided data
    PetBattleTeamData& trainerTeam = _teams[PET_BATTLE_TEAM_2];
    trainerTeam.PetCount = 0;

    for (size_t i = 0; i < npcTeam.size() && i < MAX_PET_BATTLE_TEAM_SIZE; ++i)
    {
        NPCTeamPetInfo const& info = npcTeam[i];
        PetBattlePetData& pet = trainerTeam.Pets[i];

        pet.Species = info.SpeciesID;
        pet.Level = info.Level;
        pet.Breed = info.BreedID;
        pet.Quality = info.Quality;

        BattlePetSpeciesEntry const* species = sBattlePetSpeciesStore.LookupEntry(info.SpeciesID);
        if (species)
        {
            pet.CreatureID = species->CreatureID;
            pet.PetType = species->PetTypeEnum;
            if (CreatureTemplate const* ct = sObjectMgr->GetCreatureTemplate(species->CreatureID))
                if (CreatureModel const* model = ct->GetRandomValidModel())
                    pet.DisplayID = model->CreatureDisplayID;
        }

        // Calculate stats
        CalculateWildPetStats(pet);

        // Assign abilities
        for (uint8 a = 0; a < MAX_PET_BATTLE_ABILITIES; ++a)
        {
            if (info.AbilityIDs[a] != 0)
                pet.AbilityIDs[a] = info.AbilityIDs[a];
        }
        // Fill any empty ability slots from DB2
        LoadWildPetAbilities(pet);

        trainerTeam.PetCount++;
    }

    // Recalculate effective stats for all pets
    for (uint8 t = 0; t < MAX_PET_BATTLE_PLAYERS; ++t)
        for (uint8 p = 0; p < _teams[t].PetCount; ++p)
            _teams[t].Pets[p].RecalculateEffectiveStats();

    _state = PET_BATTLE_STATE_WAITING_PRE_BATTLE;
}

void PetBattle::GenerateNPCTeamInput()
{
    PetBattleTeamData& npcTeam = _teams[PET_BATTLE_TEAM_2];
    PetBattlePetData& frontPet = npcTeam.Pets[npcTeam.FrontPetIndex];

    if (!frontPet.IsAlive())
    {
        int8 nextAlive = npcTeam.GetFirstAlivePetIndex();
        if (nextAlive >= 0 && nextAlive != npcTeam.FrontPetIndex)
        {
            SubmitInput(PET_BATTLE_TEAM_2, PET_BATTLE_MOVE_SWAP, 0, nextAlive);
            return;
        }
        SubmitInput(PET_BATTLE_TEAM_2, PET_BATTLE_MOVE_PASS, 0, -1);
        return;
    }

    // If locked in multi-turn, auto-continue
    if (frontPet.IsLockedByMultiTurn)
    {
        SubmitInput(PET_BATTLE_TEAM_2, PET_BATTLE_MOVE_ABILITY, frontPet.MultiTurnAbilityID, -1);
        return;
    }

    // Collect available abilities
    struct AbilityOption { uint32 abilityID; float priority; };
    std::vector<AbilityOption> availableAbilities;

    PetBattlePetData const& defenderPet = _teams[PET_BATTLE_TEAM_1].Pets[_teams[PET_BATTLE_TEAM_1].FrontPetIndex];

    for (uint8 i = 0; i < MAX_PET_BATTLE_ABILITIES; ++i)
    {
        if (frontPet.AbilityIDs[i] == 0 || frontPet.AbilityCooldowns[i] > 0)
            continue;

        float priority = 1.0f;
        BattlePetAbilityEntry const* ability = sBattlePetAbilityStore.LookupEntry(frontPet.AbilityIDs[i]);
        if (ability)
        {
            float typeMod = GetTypeEffectiveness(PetBattlePetType(ability->PetTypeEnum), PetBattlePetType(defenderPet.PetType));
            if (typeMod > 1.0f)
                priority += 3.0f; // Trainer AI strongly prefers super-effective
            else if (typeMod < 1.0f)
                priority -= 1.0f;
        }

        availableAbilities.push_back({ frontPet.AbilityIDs[i], priority });
    }

    if (availableAbilities.empty())
    {
        // NPC trainers never skip turns — use any ability even on cooldown as fallback
        for (uint8 i = 0; i < MAX_PET_BATTLE_ABILITIES; ++i)
        {
            if (frontPet.AbilityIDs[i] != 0)
            {
                SubmitInput(PET_BATTLE_TEAM_2, PET_BATTLE_MOVE_ABILITY, frontPet.AbilityIDs[i], -1);
                return;
            }
        }
        SubmitInput(PET_BATTLE_TEAM_2, PET_BATTLE_MOVE_PASS, 0, -1);
        return;
    }

    // Smart swap: if HP is low and at type disadvantage, consider swapping
    if (frontPet.Health < frontPet.MaxHealth / 3 && npcTeam.PetCount > 1)
    {
        float currentTypeMod = GetTypeEffectiveness(PetBattlePetType(frontPet.PetType), PetBattlePetType(defenderPet.PetType));
        if (currentTypeMod < 1.0f)
        {
            for (uint8 i = 0; i < npcTeam.PetCount; ++i)
            {
                if (i == npcTeam.FrontPetIndex || !npcTeam.Pets[i].IsAlive() || npcTeam.Pets[i].IsCaptured)
                    continue;

                float swapTypeMod = GetTypeEffectiveness(PetBattlePetType(npcTeam.Pets[i].PetType), PetBattlePetType(defenderPet.PetType));
                if (swapTypeMod > currentTypeMod)
                {
                    SubmitInput(PET_BATTLE_TEAM_2, PET_BATTLE_MOVE_SWAP, 0, static_cast<int8>(i));
                    return;
                }
            }
        }
    }

    // Select highest priority ability
    std::sort(availableAbilities.begin(), availableAbilities.end(),
        [](AbilityOption const& a, AbilityOption const& b) { return a.priority > b.priority; });

    SubmitInput(PET_BATTLE_TEAM_2, PET_BATTLE_MOVE_ABILITY, availableAbilities[0].abilityID, -1);
}

} // namespace PetBattles
