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
#include "Creature.h"
#include "DB2Stores.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include <algorithm>

namespace PetBattles
{

PetBattle::PetBattle()
{
    _teams = {};
}

PetBattle::~PetBattle() = default;

void PetBattle::InitWildBattle(Player* player, ObjectGuid wildCreatureGUID)
{
    _battleType = PET_BATTLE_TYPE_WILD;
    _wildCreatureGUID = wildCreatureGUID;

    PetBattleTeamData& playerTeam = _teams[PET_BATTLE_TEAM_1];
    playerTeam.PlayerGUID = player->GetGUID();

    // Load player's battle pets from their slots
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

        PetBattlePetData& battlePet = playerTeam.Pets[petIdx];
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
        battlePet.CustomName = pet->PacketInfo.Name;

        BattlePetSpeciesEntry const* species = sBattlePetSpeciesStore.LookupEntry(pet->PacketInfo.Species);
        if (species)
            battlePet.PetType = species->PetTypeEnum;

        ++petIdx;
        if (petIdx >= MAX_PET_BATTLE_TEAM_SIZE)
            break;
    }
    playerTeam.PetCount = petIdx;

    // Generate wild team from creature
    PetBattleTeamData& wildTeam = _teams[PET_BATTLE_TEAM_2];
    if (Creature* creature = ObjectAccessor::GetCreature(*player, wildCreatureGUID))
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

        // Calculate wild pet stats based on level, breed, quality
        // Simple formula: base + level * growthRate
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

        wildTeam.PetCount = 1;
    }

    _state = PET_BATTLE_STATE_WAITING_PRE_BATTLE;
}

void PetBattle::InitPvPBattle(Player* player1, Player* player2)
{
    _battleType = PET_BATTLE_TYPE_PVP;
    // PvP battle initialization similar to wild but for two players
    // Implementation follows same pattern as InitWildBattle for both teams
    _state = PET_BATTLE_STATE_WAITING_PRE_BATTLE;
}

void PetBattle::Start()
{
    _state = PET_BATTLE_STATE_WAITING_FOR_ROUND_INPUT;
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

        // Check for timeout
        if (_elapsedSecs >= PET_BATTLE_MAX_GAME_LENGTH)
        {
            FinishBattle(PET_BATTLE_RESULT_DRAW);
            return;
        }
    }

    // Check if both teams have submitted input
    if (_state == PET_BATTLE_STATE_WAITING_FOR_ROUND_INPUT && BothTeamsReady())
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

void PetBattle::ProcessRound()
{
    _state = PET_BATTLE_STATE_ROUND_IN_PROGRESS;
    _currentRound++;
    _roundEffects.clear();
    _petKilledThisRound.fill(false);
    _needsFrontPetSwap.fill(false);

    // Determine turn order based on speed
    uint8 firstTeam = PET_BATTLE_TEAM_1;
    uint8 secondTeam = PET_BATTLE_TEAM_2;
    ResolveSpeed();

    PetBattlePetData& pet1 = _teams[PET_BATTLE_TEAM_1].Pets[_teams[PET_BATTLE_TEAM_1].FrontPetIndex];
    PetBattlePetData& pet2 = _teams[PET_BATTLE_TEAM_2].Pets[_teams[PET_BATTLE_TEAM_2].FrontPetIndex];

    if (pet2.Speed > pet1.Speed)
        std::swap(firstTeam, secondTeam);

    // Process turns
    ProcessTurnForTeam(firstTeam);
    ProcessTurnForTeam(secondTeam);

    // Tick auras/DoTs
    TickAuras();

    // Check for deaths
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

    // Check if any team needs to swap front pet
    for (uint8 i = 0; i < MAX_PET_BATTLE_PLAYERS; ++i)
    {
        if (!_teams[i].Pets[_teams[i].FrontPetIndex].IsAlive())
        {
            _needsFrontPetSwap[i] = true;
            _state = PET_BATTLE_STATE_WAITING_FOR_FRONT_PET;
            return;
        }
    }

    _state = PET_BATTLE_STATE_WAITING_FOR_ROUND_INPUT;
}

void PetBattle::ProcessTurnForTeam(uint8 teamIdx)
{
    PetBattleTeamData& team = _teams[teamIdx];
    uint8 opponentTeam = teamIdx == PET_BATTLE_TEAM_1 ? PET_BATTLE_TEAM_2 : PET_BATTLE_TEAM_1;

    switch (team.PendingMoveType)
    {
        case PET_BATTLE_MOVE_USE_ABILITY:
        {
            PetBattlePetData& activePet = team.Pets[team.FrontPetIndex];
            if (!activePet.IsAlive())
                break;

            // Check ability cooldown
            for (uint8 i = 0; i < MAX_PET_BATTLE_ABILITIES; ++i)
            {
                if (activePet.AbilityIDs[i] == team.PendingAbilityID)
                {
                    if (activePet.AbilityCooldowns[i] > 0)
                        break; // On cooldown, skip turn

                    // Apply cooldown
                    if (BattlePetAbilityEntry const* ability = sBattlePetAbilityStore.LookupEntry(team.PendingAbilityID))
                        activePet.AbilityCooldowns[i] = ability->Cooldown;
                    break;
                }
            }

            // Calculate and apply damage
            PetBattleTeamData& opponent = _teams[opponentTeam];
            CalculateDamage(teamIdx, team.FrontPetIndex, opponentTeam, opponent.FrontPetIndex, team.PendingAbilityID);
            break;
        }
        case PET_BATTLE_MOVE_SWAP_PET:
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
            if (_battleType != PET_BATTLE_TYPE_WILD || teamIdx != PET_BATTLE_TEAM_1)
                break;

            PetBattleTeamData& wildTeam = _teams[PET_BATTLE_TEAM_2];
            PetBattlePetData& wildPet = wildTeam.Pets[wildTeam.FrontPetIndex];

            float healthPct = wildPet.MaxHealth > 0 ? (float(wildPet.Health) / float(wildPet.MaxHealth)) * 100.0f : 100.0f;

            // Get trap level from player's BattlePetMgr
            uint16 trapLevel = 1;
            if (Player* player = GetPlayerForTeam(teamIdx))
                trapLevel = player->GetSession()->GetBattlePetMgr()->GetTrapLevel();

            float captureChance = GetCaptureChance(trapLevel, healthPct);

            // Roll for capture
            if (float(rand()) / float(RAND_MAX) < captureChance)
            {
                wildPet.IsCaptured = true;
                wildPet.Health = 0;
                FinishBattle(PET_BATTLE_RESULT_TEAM_1_WIN);
            }
            break;
        }
        case PET_BATTLE_MOVE_FORFEIT:
        {
            Forfeit(teamIdx);
            break;
        }
        case PET_BATTLE_MOVE_SKIP_TURN:
        default:
            break;
    }
}

void PetBattle::CalculateDamage(uint8 attackerTeam, uint8 attackerPet, uint8 defenderTeam, uint8 defenderPet, uint32 abilityID)
{
    PetBattlePetData& attacker = _teams[attackerTeam].Pets[attackerPet];
    PetBattlePetData& defender = _teams[defenderTeam].Pets[defenderPet];

    if (!attacker.IsAlive() || !defender.IsAlive())
        return;

    BattlePetAbilityEntry const* ability = sBattlePetAbilityStore.LookupEntry(abilityID);
    if (!ability)
        return;

    // Base damage formula: power * (attackerPower / 20) * typeModifier
    float baseDamage = float(attacker.Power);
    float typeModifier = GetTypeEffectiveness(PetBattlePetType(ability->PetTypeEnum), PetBattlePetType(defender.PetType));

    int32 damage = int32(baseDamage * typeModifier);
    if (damage < 1)
        damage = 1;

    // Apply damage
    defender.Health = std::max(0, defender.Health - damage);

    PetBattleRoundEffect effect;
    effect.AbilityEffectID = abilityID;
    effect.EffectType = PET_BATTLE_EFFECT_SET_HEALTH;
    effect.SourceTeam = attackerTeam;
    effect.SourcePet = attackerPet;
    effect.TargetTeam = defenderTeam;
    effect.TargetPet = defenderPet;
    effect.Param1 = defender.Health;
    _roundEffects.push_back(effect);
}

void PetBattle::ResolveSpeed()
{
    // Speed ties are resolved randomly
    PetBattlePetData& pet1 = _teams[PET_BATTLE_TEAM_1].Pets[_teams[PET_BATTLE_TEAM_1].FrontPetIndex];
    PetBattlePetData& pet2 = _teams[PET_BATTLE_TEAM_2].Pets[_teams[PET_BATTLE_TEAM_2].FrontPetIndex];

    if (pet1.Speed == pet2.Speed)
    {
        // Random 50/50
        if (rand() % 2)
            pet1.Speed += 1; // temporary boost for this round's ordering
    }
}

void PetBattle::TickAuras()
{
    // Decrease ability cooldowns
    for (auto& team : _teams)
        for (uint8 p = 0; p < team.PetCount; ++p)
            for (auto& cd : team.Pets[p].AbilityCooldowns)
                if (cd > 0)
                    --cd;
}

void PetBattle::CheckDeaths()
{
    for (uint8 t = 0; t < MAX_PET_BATTLE_PLAYERS; ++t)
    {
        PetBattlePetData& frontPet = _teams[t].Pets[_teams[t].FrontPetIndex];
        if (!frontPet.IsAlive())
            _petKilledThisRound[t * MAX_PET_BATTLE_TEAM_SIZE + _teams[t].FrontPetIndex] = true;
    }
}

void PetBattle::AwardExperience()
{
    if (_battleType == PET_BATTLE_TYPE_WILD && _winnerTeam == PET_BATTLE_TEAM_1)
    {
        Player* player = GetPlayerForTeam(PET_BATTLE_TEAM_1);
        if (!player)
            return;

        // Award XP to active pet
        PetBattleTeamData& team = _teams[PET_BATTLE_TEAM_1];
        PetBattlePetData& activePet = team.Pets[team.FrontPetIndex];

        if (activePet.Level < BattlePets::MAX_BATTLE_PET_LEVEL)
        {
            uint16 xpAward = 100; // Base XP, should be calculated from opponent level
            player->GetSession()->GetBattlePetMgr()->GrantBattlePetExperience(
                activePet.BattlePetGUID, xpAward, BattlePets::BattlePetXpSource::PetBattle);
        }
    }
}

void PetBattle::FinishBattle(PetBattleResult result)
{
    _state = PET_BATTLE_STATE_FINISHED;

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
}

void PetBattle::Forfeit(uint8 teamIdx)
{
    if (teamIdx == PET_BATTLE_TEAM_1)
        FinishBattle(PET_BATTLE_RESULT_TEAM_2_WIN);
    else
        FinishBattle(PET_BATTLE_RESULT_TEAM_1_WIN);
}

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

} // namespace PetBattles
