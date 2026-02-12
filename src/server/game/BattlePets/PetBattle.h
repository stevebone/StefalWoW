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

#ifndef TRINITYCORE_PET_BATTLE_H
#define TRINITYCORE_PET_BATTLE_H

#include "PetBattleDefines.h"
#include "ObjectGuid.h"
#include <array>
#include <vector>

class Player;

namespace PetBattles
{

struct PetBattleAura
{
    uint32 AbilityID = 0;
    int32 Duration = 0;
    int32 MaxDuration = 0;
    uint32 AuraInstanceID = 0;
    int32 CurrentRound = 0;
};

struct PetBattlePetData
{
    ObjectGuid BattlePetGUID;
    uint32 Species = 0;
    uint32 CreatureID = 0;
    uint32 DisplayID = 0;
    uint16 Breed = 0;
    uint16 Level = 0;
    uint16 Xp = 0;
    uint8 Quality = 0;
    int8 PetType = 0;
    uint32 Flags = 0;
    std::string CustomName;

    // Combat stats
    int32 Health = 0;
    int32 MaxHealth = 0;
    int32 Power = 0;
    int32 Speed = 0;

    // Ability IDs (up to 3 per pet)
    std::array<uint32, MAX_PET_BATTLE_ABILITIES> AbilityIDs = {};
    std::array<int8, MAX_PET_BATTLE_ABILITIES> AbilityCooldowns = {};

    // Battle state
    bool IsAlive() const { return Health > 0; }
    bool IsCaptured = false;

    // Auras applied to this pet
    std::vector<PetBattleAura> Auras;

    // State values for the pet
    std::vector<std::pair<uint32, int32>> States;
};

struct PetBattleTeamData
{
    ObjectGuid PlayerGUID;
    uint32 TrapAbilityID = 0;
    uint16 TrapStatus = 0;      // 0 = not used, 1 = used, 2+ = cooldown
    int8 FrontPetIndex = 0;     // index of current active pet [0..2]
    uint8 InputFlags = 0;
    bool HasInputThisRound = false;

    std::array<PetBattlePetData, MAX_PET_BATTLE_TEAM_SIZE> Pets;
    uint8 PetCount = 0;

    // Pending input for this round
    PetBattleMoveType PendingMoveType = PET_BATTLE_MOVE_SKIP_TURN;
    uint32 PendingAbilityID = 0;
    int8 PendingNewFrontPet = -1;

    bool HasAlivePets() const
    {
        for (uint8 i = 0; i < PetCount; ++i)
            if (Pets[i].IsAlive() && !Pets[i].IsCaptured)
                return true;
        return false;
    }

    int8 GetFirstAlivePetIndex() const
    {
        for (uint8 i = 0; i < PetCount; ++i)
            if (Pets[i].IsAlive() && !Pets[i].IsCaptured)
                return static_cast<int8>(i);
        return -1;
    }
};

// Represents a single effect that happened during round resolution
struct PetBattleRoundEffect
{
    uint32 AbilityEffectID = 0;
    uint8 EffectType = 0;
    uint8 SourceTeam = 0;
    uint8 SourcePet = 0;
    uint8 TargetTeam = 0;
    uint8 TargetPet = 0;
    int32 Param1 = 0;
    int32 Param2 = 0;
};

class TC_GAME_API PetBattle
{
public:
    PetBattle();
    ~PetBattle();

    uint32 GetBattleID() const { return _battleID; }
    void SetBattleID(uint32 id) { _battleID = id; }

    PetBattleType GetBattleType() const { return _battleType; }
    PetBattleState GetBattleState() const { return _state; }
    uint32 GetCurrentRound() const { return _currentRound; }
    uint32 GetElapsedTime() const { return _elapsedSecs; }

    PetBattleTeamData& GetTeam(uint8 teamIdx) { return _teams[teamIdx]; }
    PetBattleTeamData const& GetTeam(uint8 teamIdx) const { return _teams[teamIdx]; }

    // Setup
    void InitWildBattle(Player* player, ObjectGuid wildCreatureGUID);
    void InitPvPBattle(Player* player1, Player* player2);

    // State machine
    void Start();
    void Update(uint32 diff);
    void SubmitInput(uint8 teamIdx, PetBattleMoveType moveType, uint32 abilityID, int8 newFrontPet);
    void ProcessRound();
    void FinishBattle(PetBattleResult result);
    void Forfeit(uint8 teamIdx);

    // Queries
    bool IsFinished() const { return _state == PET_BATTLE_STATE_FINISHED; }
    bool BothTeamsReady() const;
    uint8 GetWinnerTeam() const { return _winnerTeam; }
    Player* GetPlayerForTeam(uint8 teamIdx) const;

    // Round resolution results
    std::vector<PetBattleRoundEffect> const& GetRoundEffects() const { return _roundEffects; }
    bool WasPetKilledThisRound(uint8 teamIdx, uint8 petIdx) const;
    bool NeedsFrontPetSwap(uint8 teamIdx) const;

private:
    void CalculateDamage(uint8 attackerTeam, uint8 attackerPet, uint8 defenderTeam, uint8 defenderPet, uint32 abilityID);
    void ApplyAbilityEffects(uint8 team, uint8 pet, uint32 abilityID);
    void ResolveSpeed();
    void ProcessTurnForTeam(uint8 teamIdx);
    void TickAuras();
    void CheckDeaths();
    void AwardExperience();

    uint32 _battleID = 0;
    PetBattleType _battleType = PET_BATTLE_TYPE_WILD;
    PetBattleState _state = PET_BATTLE_STATE_CREATED;
    uint32 _currentRound = 0;
    uint32 _elapsedSecs = 0;
    uint32 _updateTimer = 0;
    uint8 _winnerTeam = 0;

    std::array<PetBattleTeamData, MAX_PET_BATTLE_PLAYERS> _teams;

    // Round resolution data
    std::vector<PetBattleRoundEffect> _roundEffects;
    std::array<bool, MAX_PET_BATTLE_TEAM_SIZE * MAX_PET_BATTLE_PLAYERS> _petKilledThisRound = {};
    std::array<bool, MAX_PET_BATTLE_PLAYERS> _needsFrontPetSwap = {};

    ObjectGuid _wildCreatureGUID;
};

} // namespace PetBattles

#endif // TRINITYCORE_PET_BATTLE_H
