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

#include "Followship_bots_battleground_handler.h"
#include "Followship_bots_warsong_gulch.h"

#include "Battleground.h"
#include "BattlegroundPackets.h"
#include "BattlegroundScore.h"
#include "Creature.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptHelpers.h"
#include "Unit.h"

#include <algorithm>
#include <random>
#include <unordered_map>

namespace FSBBattleground
{
    void SpawnBots(Battleground* battleground, BattlegroundMap* /*battlegroundMap*/)
    {
        if (!battleground)
        {
            TC_LOG_INFO("scripts.fsb.battleground", "FSBBattleground::SpawnBots: null battleground");
            return;
        }

        TC_LOG_INFO("scripts.fsb.battleground", "FSBBattleground::SpawnBots: BG type {}", battleground->GetTypeID());

        switch (battleground->GetTypeID())
        {
            case BATTLEGROUND_WS:
            case BATTLEGROUND_WG_CTF:
                WarsongGulch::SpawnBots(battleground);
                break;
            default:
                TC_LOG_DEBUG("scripts.fsb.battleground", "FSBBattleground::SpawnBots not implemented for bg type {}", battleground->GetTypeID());
                break;
        }
    }

    void HandlePlayerKilledBot(ObjectGuid killerGuid, Unit* botVictim)
    {
        if (!killerGuid || !botVictim)
            return;

        BattlegroundMap* bgMap = botVictim->GetMap()->ToBattlegroundMap();
        if (!bgMap)
            return;

        Battleground* bg = bgMap->GetBG();
        if (!bg)
            return;

        Player* killer = ObjectAccessor::GetPlayer(bgMap, killerGuid);
        if (!killer)
            return;

        bg->UpdatePlayerScore(killer, SCORE_KILLING_BLOWS, 1);
        bg->UpdatePlayerScore(killer, SCORE_HONORABLE_KILLS, 1);
    }

    void HandleBotKilledPlayer(Unit* botKiller, ObjectGuid victimGuid)
    {
        if (!botKiller || !victimGuid)
            return;

        BattlegroundMap* bgMap = botKiller->GetMap()->ToBattlegroundMap();
        if (!bgMap)
            return;

        Battleground* bg = bgMap->GetBG();
        if (!bg)
            return;

        Player* victim = ObjectAccessor::GetPlayer(bgMap, victimGuid);
        if (!victim)
            return;

        bg->UpdatePlayerScore(victim, SCORE_DEATHS, 1);
    }

    namespace Impl
    {
        std::unordered_map<BattlegroundMap*, std::vector<ObjectGuid>> SpawnedBotGuids;
    }

    void AddBotSpawnGuid(BattlegroundMap* battlegroundMap, ObjectGuid guid)
    {
        Impl::SpawnedBotGuids[battlegroundMap].push_back(guid);
    }

    void ClearSpawnedBotGuids(BattlegroundMap* battlegroundMap)
    {
        Impl::SpawnedBotGuids.erase(battlegroundMap);
    }

    uint32 CountExistingBots(Battleground* battleground, Team team)
    {
        BattlegroundMap* bgMap = battleground->GetBgMap();
        if (!bgMap)
            return 0;

        uint32 count = 0;
        for (auto const& [guid, creature] : bgMap->GetObjectsStore().Data.Head)
        {
            if (!creature || !creature->IsBot())
                continue;

            FSB_Race race = FSBMgr::Get()->GetBotRaceForEntry(creature->GetEntry());
            if (FSBUtils::GetTeamFromFSBRace(race) == team)
                ++count;
        }

        return count;
    }

    std::vector<uint32> SelectRandomEntries(std::vector<uint32>& entries, uint32 count)
    {
        std::shuffle(entries.begin(), entries.end(), std::mt19937(std::random_device{}()));
        if (count > entries.size())
            count = static_cast<uint32>(entries.size());

        return std::vector<uint32>(entries.begin(), entries.begin() + count);
    }

    std::vector<ObjectGuid> const& GetSpawnedBotGuids(BattlegroundMap* battlegroundMap)
    {
        static std::vector<ObjectGuid> const empty;
        auto it = Impl::SpawnedBotGuids.find(battlegroundMap);
        return it != Impl::SpawnedBotGuids.end() ? it->second : empty;
    }

    void OnBuildPvPLogDataPacket(BattlegroundMap* battlegroundMap, WorldPackets::Battleground::PVPMatchStatistics& pvpLogData)
    {
        if (!battlegroundMap)
            return;

        auto it = Impl::SpawnedBotGuids.find(battlegroundMap);
        if (it == Impl::SpawnedBotGuids.end())
            return;

        int8 botCountAlliance = 0;
        int8 botCountHorde = 0;

        for (ObjectGuid botGuid : it->second)
        {
            Creature* bot = battlegroundMap->GetCreature(botGuid);
            if (!bot)
                continue;

            FSB_Race botRace = FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry());
            FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());
            Gender botGender = FSBMgr::Get()->GetBotGenderForEntry(bot->GetEntry());
            Team team = FSBUtils::GetTeamFromFSBRace(botRace);

            if (team == ALLIANCE)
                ++botCountAlliance;
            else
                ++botCountHorde;

            ScriptHelpers::AddCreatureToPvPLogData(pvpLogData, botGuid, uint8(FSBUtils::BotRaceToTC(botRace)), uint8(FSBUtils::FSBToTCClass(botClass)), botGender, bot->GetEntry(), team);
        }

        pvpLogData.PlayerCount[PVP_TEAM_ALLIANCE] += botCountAlliance;
        pvpLogData.PlayerCount[PVP_TEAM_HORDE] += botCountHorde;
    }
}
