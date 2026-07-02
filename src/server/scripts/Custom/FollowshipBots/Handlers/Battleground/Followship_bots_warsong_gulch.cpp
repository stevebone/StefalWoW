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

#include "Followship_bots_warsong_gulch.h"

#include "Battleground.h"
#include "BattlegroundScript.h"
#include "BattlegroundPackets.h"
#include "Creature.h"
#include "DBCEnums.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"
#include "Log.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Random.h"
#include "ScriptHelpers.h"

#include <algorithm>
#include <random>
#include <unordered_map>
#include <vector>

namespace
{
    constexpr uint32 FSB_WSG_MAX_BOTS_PER_TEAM = 10;
    std::unordered_map<BattlegroundMap*, std::vector<ObjectGuid>> SpawnedBotGuids;

    Team GetTeamFromFSBRace(FSB_Race race)
    {
        switch (race)
        {
            case FSB_Race::Human:
            case FSB_Race::Dwarf:
            case FSB_Race::NightElf:
            case FSB_Race::Gnome:
            case FSB_Race::Draenei:
            case FSB_Race::Worgen:
            case FSB_Race::Pandaren:
            case FSB_Race::VoidElf:
            case FSB_Race::HighmountainTauren:
            case FSB_Race::Nightborne:
            case FSB_Race::LightforgedDraenei:
            case FSB_Race::EarthenAlliance:
            case FSB_Race::HaranirAlliance:
                return Team::ALLIANCE;

            case FSB_Race::Orc:
            case FSB_Race::Undead:
            case FSB_Race::Tauren:
            case FSB_Race::Troll:
            case FSB_Race::BloodElf:
            case FSB_Race::Goblin:
            case FSB_Race::PandarenHorde:
            case FSB_Race::EarthenHorde:
            case FSB_Race::HaranirHorde:
                return Team::HORDE;

            default:
                return Team::PANDARIA_NEUTRAL;
        }
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
            if (GetTeamFromFSBRace(race) == team)
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
}

namespace FSBBattleground::WarsongGulch
{
    void SpawnBots(Battleground* battleground)
    {
        TC_LOG_INFO("scripts.fsb.battleground", "FSB WSG Handler: SpawnBots entered");

        SpawnedBotGuids[battleground->GetBgMap()].clear();

        uint32 const alliancePlayers = battleground->GetPlayersCountByTeam(ALLIANCE);
        uint32 const hordePlayers = battleground->GetPlayersCountByTeam(HORDE);
        uint32 const allianceExistingBots = CountExistingBots(battleground, ALLIANCE);
        uint32 const hordeExistingBots = CountExistingBots(battleground, HORDE);

        int32 const allianceNeeded = std::max(0, int32(FSB_WSG_MAX_BOTS_PER_TEAM) - int32(alliancePlayers) - int32(allianceExistingBots));
        int32 const hordeNeeded = std::max(0, int32(FSB_WSG_MAX_BOTS_PER_TEAM) - int32(hordePlayers) - int32(hordeExistingBots));

        TC_LOG_INFO("scripts.fsb.battleground", "FSB WSG Handler: players A={} H={}; existing bots A={} H={}; needed A={} H={}",
            alliancePlayers, hordePlayers, allianceExistingBots, hordeExistingBots, allianceNeeded, hordeNeeded);

        if (allianceNeeded <= 0 && hordeNeeded <= 0)
        {
            TC_LOG_INFO("scripts.fsb.battleground", "FSB WSG Handler: No bots needed, aborting");
            return;
        }

        std::vector<uint32> allianceEntries;
        std::vector<uint32> hordeEntries;

        uint32 totalTemplates = 0;
        uint32 hiredTemplates = 0;
        for (auto const& [entry, templateData] : FSBMgr::Get()->GetBotTemplates())
        {
            ++totalTemplates;
            if (FSBMgr::Get()->IsBotTemplateHired(entry))
            {
                ++hiredTemplates;
                continue;
            }

            Team team = GetTeamFromFSBRace(templateData.botRace);
            if (team == ALLIANCE)
                allianceEntries.push_back(entry);
            else if (team == HORDE)
                hordeEntries.push_back(entry);
        }

        TC_LOG_INFO("scripts.fsb.battleground", "FSB WSG Handler: bot templates total={} hired={} available A={} H={}",
            totalTemplates, hiredTemplates, allianceEntries.size(), hordeEntries.size());

        auto const spawnTeam = [&](Team team, int32 needed, std::vector<uint32>& availableEntries)
        {
            if (needed <= 0)
                return;

            std::vector<uint32> selected = SelectRandomEntries(availableEntries, static_cast<uint32>(needed));
            if (selected.empty())
            {
                TC_LOG_WARN("scripts.fsb.battleground", "FSB WSG Handler: No available bot templates for team {}", team);
                return;
            }

            TC_LOG_INFO("scripts.fsb.battleground", "FSB WSG Handler: selected {} bot entries for team {}", selected.size(), team);

            TeamId teamId = Battleground::GetTeamIndexByTeamId(team);
            WorldSafeLocsEntry const* startPos = battleground->GetTeamStartPosition(teamId);
            if (!startPos)
            {
                TC_LOG_ERROR("scripts.fsb.battleground", "FSB WSG Handler: No start position for team {}", team);
                return;
            }

            TC_LOG_INFO("scripts.fsb.battleground", "FSB WSG Handler: start position for team {} found", team);

            for (uint32 entry : selected)
            {
                Position pos(startPos->Loc);
                pos.m_positionX += frand(-3.0f, 3.0f);
                pos.m_positionY += frand(-3.0f, 3.0f);

                Creature* bot = battleground->GetBgMap()->SummonCreature(entry, pos);
                if (!bot)
                {
                    TC_LOG_ERROR("scripts.fsb.battleground", "FSB WSG Handler: Failed to summon bot entry {}", entry);
                    continue;
                }

                bot->SetPvP(true);
                SpawnedBotGuids[battleground->GetBgMap()].push_back(bot->GetGUID());

                TC_LOG_INFO("scripts.fsb.battleground", "FSB WSG Handler: Spawned bot {} for team {}", bot->GetEntry(), team);
            }

            TC_LOG_INFO("scripts.fsb.battleground", "FSB WSG Handler: Spawned bots for team {}", team);
        };

        spawnTeam(ALLIANCE, allianceNeeded, allianceEntries);
        spawnTeam(HORDE, hordeNeeded, hordeEntries);
    }

    std::vector<ObjectGuid> const& GetSpawnedBotGuids(BattlegroundMap* battlegroundMap)
    {
        static std::vector<ObjectGuid> const empty;
        auto it = SpawnedBotGuids.find(battlegroundMap);
        return it != SpawnedBotGuids.end() ? it->second : empty;
    }

    void OnBuildPvPLogDataPacket(BattlegroundMap* battlegroundMap, WorldPackets::Battleground::PVPMatchStatistics& pvpLogData)
    {
        if (!battlegroundMap)
            return;

        auto it = SpawnedBotGuids.find(battlegroundMap);
        if (it == SpawnedBotGuids.end())
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
            Team team = GetTeamFromFSBRace(botRace);

            if (team == ALLIANCE)
                ++botCountAlliance;
            else
                ++botCountHorde;

            ScriptHelpers::AddCreatureToPvPLogData(pvpLogData, botGuid, uint8(FSBUtils::BotRaceToTC(botRace)), uint8(FSBUtils::FSBToTCClass(botClass)), botGender, bot->GetEntry(), team);
        }

        pvpLogData.PlayerCount[PVP_TEAM_ALLIANCE] += botCountAlliance;
        pvpLogData.PlayerCount[PVP_TEAM_HORDE] += botCountHorde;
    }

    void ClearSpawnedBotGuids(BattlegroundMap* battlegroundMap)
    {
        SpawnedBotGuids.erase(battlegroundMap);
    }
}
