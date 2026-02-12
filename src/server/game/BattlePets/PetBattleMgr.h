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

#ifndef TRINITYCORE_PET_BATTLE_MGR_H
#define TRINITYCORE_PET_BATTLE_MGR_H

#include "PetBattle.h"
#include <memory>
#include <unordered_map>

class Player;

namespace PetBattles
{

class TC_GAME_API PetBattleMgr
{
public:
    PetBattleMgr();
    ~PetBattleMgr();

    static PetBattleMgr* instance();

    void Initialize();
    void Update(uint32 diff);

    // Battle lifecycle
    PetBattle* CreateWildBattle(Player* player, ObjectGuid wildCreatureGUID);
    PetBattle* CreatePvPBattle(Player* player1, Player* player2);
    void RemoveBattle(uint32 battleID);

    // Lookups
    PetBattle* GetBattleByID(uint32 battleID);
    PetBattle* GetBattleByPlayer(ObjectGuid playerGUID);
    bool IsPlayerInBattle(ObjectGuid playerGUID) const;

    // DB2 index maps built during Initialize()
    std::vector<uint32> const* GetSpeciesAbilities(uint32 speciesID) const;
    std::vector<uint32> const* GetAbilityTurns(uint32 abilityID) const;
    std::vector<uint32> const* GetTurnEffects(uint32 turnID) const;

private:
    uint32 _nextBattleID = 1;
    std::unordered_map<uint32, std::unique_ptr<PetBattle>> _activeBattles;
    std::unordered_map<ObjectGuid, uint32> _playerToBattle;  // playerGUID -> battleID

    // DB2 index maps
    std::unordered_map<uint32, std::vector<uint32>> _speciesAbilities;   // speciesID -> abilityIDs
    std::unordered_map<uint32, std::vector<uint32>> _abilityTurns;       // abilityID -> turnIDs
    std::unordered_map<uint32, std::vector<uint32>> _turnEffects;        // turnID -> effectIDs
};

} // namespace PetBattles

#define sPetBattleMgr PetBattles::PetBattleMgr::instance()

#endif // TRINITYCORE_PET_BATTLE_MGR_H
