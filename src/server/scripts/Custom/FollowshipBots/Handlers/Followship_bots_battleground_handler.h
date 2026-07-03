/*
 * This file is part of the Stefal WoW Project.
 * It is designed to work exclusively with the TrinityCore framework.
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
 * This code is provided for personal and educational use within the
 * Stefal WoW Project. It is not intended for commercial distribution,
 * resale, or any form of monetization.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"
#include <vector>

class Battleground;
class BattlegroundMap;
class Creature;
class Player;
class Unit;

namespace WorldPackets::Battleground
{
    struct PVPMatchStatistics;
}

namespace FSBBattleground
{
    // Data index used with BattlegroundScript::GetData / SetData
    constexpr uint32 BotsSpawnedDataId = 0;

    struct BotScoreData
    {
        uint32 KillingBlows = 0;
        uint32 HonorableKills = 0;
        uint32 Deaths = 0;
        uint32 DamageDone = 0;
        uint32 HealingDone = 0;
    };

    void SpawnBots(Battleground* battleground, BattlegroundMap* battlegroundMap, Player* triggeringPlayer = nullptr);

    bool IsInBG(Creature const* bot);
    bool IsInProgress(Creature const* bot);
    bool IsFinished(Creature const* bot);

    void HandlePlayerKilledBot(ObjectGuid killerGuid, Unit* botVictim);
    void HandleBotKilledPlayer(Unit* botKiller, ObjectGuid victimGuid);
    void HandlePlayerDamagedBot(Unit* attacker, Unit* botVictim, uint32 damage);

    void RecordBotKillingBlow(Unit* bot);
    void RecordBotDeath(Unit* bot);
    void RecordBotDamageDone(Unit* bot, uint32 damage);
    void RecordBotHealingDone(Unit* bot, uint32 heal);
    BotScoreData const* GetBotScore(ObjectGuid botGuid);

    void OnBuildPvPLogDataPacket(BattlegroundMap* battlegroundMap, WorldPackets::Battleground::PVPMatchStatistics& pvpLogData);
    std::vector<ObjectGuid> const& GetSpawnedBotGuids(BattlegroundMap* battlegroundMap);
    void ClearSpawnedBotGuids(BattlegroundMap* battlegroundMap);

    void AddBotSpawnGuid(BattlegroundMap* battlegroundMap, ObjectGuid guid);
    uint32 CountExistingBots(Battleground* battleground, Team team);
    std::vector<uint32> SelectRandomEntries(std::vector<uint32>& entries, uint32 count);

    std::vector<Creature*> CollectBotsOnTeam(BattlegroundMap* battlegroundMap, Team team);
    void SendRaidUpdateToPlayer(Player* player);
    void PeriodicRaidUpdate(BattlegroundMap* battlegroundMap);
}
