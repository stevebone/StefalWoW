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

class Creature;
class Player;
struct BattlePetAbilityEffectEntry;

namespace PetBattles
{

struct NPCTeamPetInfo;

struct DamageResult
{
    int32 Damage = 0;
    bool IsCrit = false;
    float TypeMod = 1.0f;
};

struct PetBattleAura
{
    uint32 AbilityID = 0;
    uint32 EffectID = 0;            // BattlePetAbilityEffectEntry::ID that triggers each tick
    int32 Duration = 0;             // total duration in rounds
    int32 RemainingRounds = 0;      // rounds until expiry
    uint32 AuraInstanceID = 0;      // unique ID for this aura instance
    int32 CurrentRound = 0;         // round when aura was applied
    uint8 CasterTeam = 0;
    uint8 CasterPet = 0;
    PetBattleAuraType AuraType = PET_BATTLE_AURA_BUFF;
    int32 DamagePerTick = 0;        // damage or healing per tick
    int8 PetType = -1;              // pet type of the caster for type effectiveness on DoTs
    uint8 StateFlags = PET_BATTLE_AURA_STATE_NONE;
};

struct PetBattleEnvironment
{
    uint32 AbilityID = 0;
    PetBattleWeatherType WeatherType = PET_BATTLE_WEATHER_NONE;
    int8 RemainingRounds = 0;
    uint8 CasterTeam = 0;
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

    // Combat stats (base stats used for calculation)
    int32 Health = 0;
    int32 MaxHealth = 0;
    int32 Power = 0;
    int32 Speed = 0;

    // Effective stats (base + aura modifiers, recalculated each round)
    int32 EffectivePower = 0;
    int32 EffectiveSpeed = 0;

    // Ability IDs (up to 3 per pet)
    std::array<uint32, MAX_PET_BATTLE_ABILITIES> AbilityIDs = {};
    std::array<int8, MAX_PET_BATTLE_ABILITIES> AbilityCooldowns = {};
    std::array<int8, MAX_PET_BATTLE_ABILITIES> AbilityLockdowns = {};

    // Multi-turn ability state
    int32 MultiTurnAbilityID = 0;       // currently executing multi-turn ability (0 = none)
    int8 MultiTurnCurrentIndex = 0;     // which turn index we're on (0-based)
    int8 MultiTurnTotalTurns = 0;       // total turns in the sequence
    bool IsLockedByMultiTurn = false;    // cannot use other abilities during multi-turn

    // Battle state
    bool IsAlive() const { return Health > 0; }
    bool IsCaptured = false;
    bool IsStunned = false;             // skips next turn

    // Passive ability state
    bool UndeadReviveUsed = false;      // Undead: already used revive
    bool MechanicalReviveUsed = false;  // Mechanical: already used revive
    bool IsUndeadReviving = false;      // Undead: currently in revive round
    bool DragonkinDamageBonus = false;  // Dragonkin: active for current round

    // Auras applied to this pet
    std::vector<PetBattleAura> Auras;

    // State values for the pet
    std::vector<std::pair<uint32, int32>> States;

    void RecalculateEffectiveStats()
    {
        EffectivePower = Power;
        EffectiveSpeed = Speed;

        for (PetBattleAura const& aura : Auras)
        {
            if (aura.AuraType == PET_BATTLE_AURA_BUFF || aura.AuraType == PET_BATTLE_AURA_DEBUFF)
            {
                // Stat modifying auras would adjust EffectivePower/EffectiveSpeed
                // The actual adjustment is done when applying the aura effect
            }
        }

        // Flying passive: +50% speed while above 50% HP
        if (PetType == PET_TYPE_FLYING && Health > MaxHealth / 2)
            EffectiveSpeed = int32(EffectiveSpeed * (1.0f + PASSIVE_FLYING_SPEED_BONUS));

        if (EffectivePower < 1) EffectivePower = 1;
        if (EffectiveSpeed < 1) EffectiveSpeed = 1;
    }
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
    uint16 Flags = 0;
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
    bool CanAwardXP() const { return _canAwardXP; }

    PetBattleTeamData& GetTeam(uint8 teamIdx) { return _teams[teamIdx]; }
    PetBattleTeamData const& GetTeam(uint8 teamIdx) const { return _teams[teamIdx]; }

    PetBattleEnvironment const& GetEnvironment(uint8 slot) const { return _environments[slot]; }

    // Setup
    void InitWildBattle(Player* player, ObjectGuid wildCreatureGUID);
    void InitPvPBattle(Player* player1, Player* player2);
    void InitNPCBattle(Player* player, Creature* trainer, std::vector<NPCTeamPetInfo> const& npcTeam);

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

    // Trap validation
    uint8 GetTrapStatus(uint8 playerTeam) const;

    // AI for wild/NPC teams
    void GenerateWildTeamInput();
    void GenerateNPCTeamInput();

    // Round resolution results
    std::vector<PetBattleRoundEffect> const& GetRoundEffects() const { return _roundEffects; }
    bool WasPetKilledThisRound(uint8 teamIdx, uint8 petIdx) const;
    bool NeedsFrontPetSwap(uint8 teamIdx) const;

private:
    // DB2-driven ability effect chain
    void ApplyAbilityEffects(uint8 attackerTeam, uint8 attackerPet, uint32 abilityID);
    void ProcessEffect(BattlePetAbilityEffectEntry const* effect, uint8 attackerTeam, uint8 attackerPet,
        uint8 defenderTeam, uint8 defenderPet, uint32 abilityID);

    // Damage/healing with passive family abilities applied
    DamageResult CalculateAbilityDamage(int32 abilityPower, int32 attackerPower, PetBattlePetType abilityType,
        PetBattlePetData const& attacker, PetBattlePetData& defender);
    int32 CalculateAbilityHealing(int32 healPower, int32 attackerPower, PetBattlePetData const& healer);

    // Aura system
    void AddAura(uint8 targetTeam, uint8 targetPet, uint32 abilityID, uint32 effectID,
        PetBattleAuraType auraType, int32 duration, int32 damagePerTick, int8 petType,
        uint8 casterTeam, uint8 casterPet);
    void RemoveAura(uint8 targetTeam, uint8 targetPet, uint32 abilityID);
    void TickAuras();

    // Environment/weather
    void SetWeather(PetBattleWeatherType weatherType, uint32 abilityID, int8 duration, uint8 casterTeam);
    void TickWeather();
    float GetWeatherDamageModifier(PetBattlePetType abilityType) const;
    float GetWeatherHealingModifier() const;

    // Passive family abilities
    void ApplyPassiveRoundStart();
    void ApplyPassiveOnDamageDealt(uint8 attackerTeam, uint8 attackerPet, uint8 defenderTeam, uint8 defenderPet, int32 damage);
    bool ApplyPassiveOnDeath(uint8 teamIdx, uint8 petIdx);

    void ResolveSpeed();
    void ProcessTurnForTeam(uint8 teamIdx);
    void CheckDeaths();
    void AwardExperience();

    // Helper to load player's battle pet team
    void LoadPlayerTeam(Player* player, PetBattleTeamData& team);
    void LoadWildPetAbilities(PetBattlePetData& pet);
    void GenerateWildTeam(Player* player, ObjectGuid wildCreatureGUID);

    uint32 _battleID = 0;
    PetBattleType _battleType = PET_BATTLE_TYPE_WILD;
    PetBattleState _state = PET_BATTLE_STATE_CREATED;
    uint32 _currentRound = 0;
    uint32 _elapsedSecs = 0;
    uint32 _updateTimer = 0;
    uint8 _winnerTeam = 0;
    bool _canAwardXP = true;
    bool _maxLengthWarningSent = false;
    uint32 _nextAuraInstanceID = 1;

    std::array<PetBattleTeamData, MAX_PET_BATTLE_PLAYERS> _teams;
    std::array<PetBattleEnvironment, MAX_PET_BATTLE_ENVIRONMENTS> _environments;

    // Round resolution data
    std::vector<PetBattleRoundEffect> _roundEffects;
    std::array<bool, MAX_PET_BATTLE_TEAM_SIZE * MAX_PET_BATTLE_PLAYERS> _petKilledThisRound = {};
    std::array<bool, MAX_PET_BATTLE_PLAYERS> _needsFrontPetSwap = {};

    ObjectGuid _wildCreatureGUID;
};

} // namespace PetBattles

#endif // TRINITYCORE_PET_BATTLE_H
