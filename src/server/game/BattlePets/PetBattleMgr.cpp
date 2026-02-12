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

#include "PetBattleMgr.h"
#include "DB2Stores.h"
#include "Log.h"
#include "Player.h"

namespace PetBattles
{

PetBattleMgr::PetBattleMgr() = default;
PetBattleMgr::~PetBattleMgr() = default;

PetBattleMgr* PetBattleMgr::instance()
{
    static PetBattleMgr instance;
    return &instance;
}

void PetBattleMgr::Initialize()
{
    TC_LOG_INFO("server.loading", ">> Loading Pet Battle data...");

    // Build speciesID -> abilityIDs index
    // This will be populated when BattlePetSpeciesXAbility DB2 is loaded
    // For now, build from available DB2 stores if loaded

    TC_LOG_INFO("server.loading", ">> Pet Battle system initialized");
}

void PetBattleMgr::Update(uint32 diff)
{
    // Update all active battles
    std::vector<uint32> finishedBattles;
    for (auto& [battleID, battle] : _activeBattles)
    {
        battle->Update(diff);
        if (battle->IsFinished())
            finishedBattles.push_back(battleID);
    }

    // Clean up finished battles after iteration
    for (uint32 battleID : finishedBattles)
        RemoveBattle(battleID);
}

PetBattle* PetBattleMgr::CreateWildBattle(Player* player, ObjectGuid wildCreatureGUID)
{
    if (IsPlayerInBattle(player->GetGUID()))
        return nullptr;

    uint32 battleID = _nextBattleID++;
    auto battle = std::make_unique<PetBattle>();
    battle->SetBattleID(battleID);
    battle->InitWildBattle(player, wildCreatureGUID);

    PetBattle* ptr = battle.get();
    _activeBattles[battleID] = std::move(battle);
    _playerToBattle[player->GetGUID()] = battleID;

    TC_LOG_DEBUG("server.loading", "PetBattleMgr: Created wild battle {} for player {}",
        battleID, player->GetGUID().ToString());

    return ptr;
}

PetBattle* PetBattleMgr::CreatePvPBattle(Player* player1, Player* player2)
{
    if (IsPlayerInBattle(player1->GetGUID()) || IsPlayerInBattle(player2->GetGUID()))
        return nullptr;

    uint32 battleID = _nextBattleID++;
    auto battle = std::make_unique<PetBattle>();
    battle->SetBattleID(battleID);
    battle->InitPvPBattle(player1, player2);

    PetBattle* ptr = battle.get();
    _activeBattles[battleID] = std::move(battle);
    _playerToBattle[player1->GetGUID()] = battleID;
    _playerToBattle[player2->GetGUID()] = battleID;

    return ptr;
}

void PetBattleMgr::RemoveBattle(uint32 battleID)
{
    auto it = _activeBattles.find(battleID);
    if (it == _activeBattles.end())
        return;

    PetBattle* battle = it->second.get();

    // Remove player mappings
    for (uint8 i = 0; i < MAX_PET_BATTLE_PLAYERS; ++i)
    {
        ObjectGuid playerGUID = battle->GetTeam(i).PlayerGUID;
        if (!playerGUID.IsEmpty())
            _playerToBattle.erase(playerGUID);
    }

    _activeBattles.erase(it);
}

PetBattle* PetBattleMgr::GetBattleByID(uint32 battleID)
{
    auto it = _activeBattles.find(battleID);
    return it != _activeBattles.end() ? it->second.get() : nullptr;
}

PetBattle* PetBattleMgr::GetBattleByPlayer(ObjectGuid playerGUID)
{
    auto it = _playerToBattle.find(playerGUID);
    if (it == _playerToBattle.end())
        return nullptr;
    return GetBattleByID(it->second);
}

bool PetBattleMgr::IsPlayerInBattle(ObjectGuid playerGUID) const
{
    return _playerToBattle.contains(playerGUID);
}

std::vector<uint32> const* PetBattleMgr::GetSpeciesAbilities(uint32 speciesID) const
{
    auto it = _speciesAbilities.find(speciesID);
    return it != _speciesAbilities.end() ? &it->second : nullptr;
}

std::vector<uint32> const* PetBattleMgr::GetAbilityTurns(uint32 abilityID) const
{
    auto it = _abilityTurns.find(abilityID);
    return it != _abilityTurns.end() ? &it->second : nullptr;
}

std::vector<uint32> const* PetBattleMgr::GetTurnEffects(uint32 turnID) const
{
    auto it = _turnEffects.find(turnID);
    return it != _turnEffects.end() ? &it->second : nullptr;
}

} // namespace PetBattles
