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

#include "Followship_bots_party_handler.h"

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
    void SpawnBots(Battleground* battleground, BattlegroundMap* /*battlegroundMap*/, Player* triggeringPlayer)
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
                WarsongGulch::SpawnBots(battleground, triggeringPlayer);
                break;
            default:
                TC_LOG_DEBUG("scripts.fsb.battleground", "FSBBattleground::SpawnBots not implemented for bg type {}", battleground->GetTypeID());
                break;
        }
    }

    bool IsInBG(Creature const* bot)
    {
        if (!bot)
            return false;

        BattlegroundMap* bgMap = bot->GetMap()->ToBattlegroundMap();
        if (!bgMap)
            return false;

        return true;
    }

    bool IsInProgress(Creature const* bot)
    {
        if (!bot)
            return false;

        BattlegroundMap* bgMap = bot->GetMap()->ToBattlegroundMap();
        if (!bgMap)
            return false;

        Battleground* bg = bgMap->GetBG();
        if (!bg)
            return false;

        return bg->GetStatus() == STATUS_IN_PROGRESS;
    }

    bool IsFinished(Creature const* bot)
    {
        if (!bot)
            return false;

        BattlegroundMap* bgMap = bot->GetMap()->ToBattlegroundMap();
        if (!bgMap)
            return false;

        Battleground* bg = bgMap->GetBG();
        if (!bg)
            return false;

        return bg->GetStatus() == STATUS_WAIT_LEAVE;
    }

    void InitializeBot(Creature* bot)
    {
        if (!bot)
            return;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return;

        if (baseAI->botHired)
            return;

        BattlegroundMap* bgMap = bot->GetMap()->ToBattlegroundMap();
        if (!bgMap)
            return;

        Battleground* bg = bgMap->GetBG();
        if (!bg)
            return;

        FSB_BattlegroundData* bgData = baseAI->GetBattlegroundData();

        if (!bgData->initialized)
        {
            bgData->bgTypeId = bg->GetTypeID();
            bgData->initialized = true;
        }

        switch (bg->GetTypeID())
        {
        case BATTLEGROUND_WS:
        case BATTLEGROUND_WG_CTF:
            WarsongGulch::InitializeBot(bot, bgData, bg);
            break;
        default:
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

    void HandlePlayerDamagedBot(Unit* attacker, Unit* botVictim, uint32 damage)
    {
        if (!attacker || !botVictim || !damage)
            return;

        Player* player = attacker->ToPlayer();
        if (!player)
            return;

        BattlegroundMap* bgMap = botVictim->GetMap()->ToBattlegroundMap();
        if (!bgMap)
            return;

        Battleground* bg = bgMap->GetBG();
        if (!bg || bg->GetStatus() != STATUS_IN_PROGRESS)
            return;

        bg->UpdatePlayerScore(player, SCORE_DAMAGE_DONE, damage);
    }

    namespace Impl
    {
        std::unordered_map<BattlegroundMap*, std::vector<ObjectGuid>> SpawnedBotGuids;
        std::unordered_map<ObjectGuid, BotScoreData> BotScores;
    }

    void RecordBotKillingBlow(Unit* bot)
    {
        if (!bot)
            return;

        BotScoreData& score = Impl::BotScores[bot->GetGUID()];
        ++score.KillingBlows;
        ++score.HonorableKills;
    }

    void RecordBotDeath(Unit* bot)
    {
        if (!bot)
            return;

        BotScoreData& score = Impl::BotScores[bot->GetGUID()];
        ++score.Deaths;
    }

    void RecordBotDamageDone(Unit* bot, uint32 damage)
    {
        if (!bot || !damage)
            return;

        BotScoreData& score = Impl::BotScores[bot->GetGUID()];
        score.DamageDone += damage;
    }

    void RecordBotHealingDone(Unit* bot, uint32 heal)
    {
        if (!bot || !heal)
            return;

        BotScoreData& score = Impl::BotScores[bot->GetGUID()];
        score.HealingDone += heal;
    }

    BotScoreData const* GetBotScore(ObjectGuid botGuid)
    {
        auto it = Impl::BotScores.find(botGuid);
        return it != Impl::BotScores.end() ? &it->second : nullptr;
    }

    void AddBotSpawnGuid(BattlegroundMap* battlegroundMap, ObjectGuid guid)
    {
        Impl::SpawnedBotGuids[battlegroundMap].push_back(guid);
    }

    void ClearSpawnedBotGuids(BattlegroundMap* battlegroundMap)
    {
        auto it = Impl::SpawnedBotGuids.find(battlegroundMap);
        if (it != Impl::SpawnedBotGuids.end())
        {
            for (ObjectGuid botGuid : it->second)
                Impl::BotScores.erase(botGuid);
        }

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

    std::vector<Creature*> CollectBotsOnTeam(BattlegroundMap* battlegroundMap, Team team)
    {
        std::vector<Creature*> bots;
        if (!battlegroundMap)
            return bots;

        for (auto const& [guid, creature] : battlegroundMap->GetObjectsStore().Data.Head)
        {
            if (!creature || !creature->IsBot())
                continue;

            FSB_Race race = FSBMgr::Get()->GetBotRaceForEntry(creature->GetEntry());
            if (FSBUtils::GetTeamFromFSBRace(race) != team)
                continue;

            // Skip bots that would produce malformed roster/member-state packets.
            if (creature->GetGUID().IsEmpty() || creature->GetName().empty())
            {
                TC_LOG_WARN("scripts.fsb.battleground", "FSBBattleground::CollectBotsOnTeam: skipping bot entry {} with empty GUID or name", creature->GetEntry());
                continue;
            }

            FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(creature->GetEntry());
            if (FSBUtils::BotRaceToTC(race) == RACE_NONE || FSBUtils::FSBToTCClass(botClass) == CLASS_NONE)
            {
                TC_LOG_WARN("scripts.fsb.battleground", "FSBBattleground::CollectBotsOnTeam: skipping bot entry {} with invalid race/class mapping", creature->GetEntry());
                continue;
            }

            bots.push_back(creature);

            // Defensive cap: a client raid roster cannot exceed MAX_RAID_SIZE members.
            if (bots.size() >= MAX_RAID_SIZE)
                break;
        }

        return bots;
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

        int8 botCountAlliance = 0;
        int8 botCountHorde = 0;

        for (auto const& [guid, creature] : battlegroundMap->GetObjectsStore().Data.Head)
        {
            if (!creature || !creature->IsBot())
                continue;

            FSB_Race botRace = FSBMgr::Get()->GetBotRaceForEntry(creature->GetEntry());
            FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(creature->GetEntry());
            Gender botGender = FSBMgr::Get()->GetBotGenderForEntry(creature->GetEntry());
            Team team = FSBUtils::GetTeamFromFSBRace(botRace);

            if (team == ALLIANCE)
                ++botCountAlliance;
            else if (team == HORDE)
                ++botCountHorde;
            else
                continue;

            uint32 killingBlows = 0;
            uint32 honorableKills = 0;
            uint32 deaths = 0;
            uint32 damageDone = 0;
            uint32 healingDone = 0;
            if (BotScoreData const* score = GetBotScore(guid))
            {
                killingBlows = score->KillingBlows;
                honorableKills = score->HonorableKills;
                deaths = score->Deaths;
                damageDone = score->DamageDone;
                healingDone = score->HealingDone;
            }

            ScriptHelpers::AddCreatureToPvPLogData(pvpLogData, guid, uint8(FSBUtils::BotRaceToTC(botRace)), uint8(FSBUtils::FSBToTCClass(botClass)), botGender, creature->GetEntry(), team, killingBlows, honorableKills, deaths, damageDone, healingDone);
        }

        pvpLogData.PlayerCount[PVP_TEAM_ALLIANCE] += botCountAlliance;
        pvpLogData.PlayerCount[PVP_TEAM_HORDE] += botCountHorde;
    }

    void SendRaidUpdateToPlayer(Player* player)
    {
        if (!player || !player->GetSession() || !player->IsInWorld())
            return;

        if (player->IsBeingTeleportedNear() || player->IsBeingTeleportedFar() || player->IsBeingTeleported())
            return;

        if (player->GetSession()->PlayerLoading())
            return;

        BattlegroundMap* bgMap = player->GetMap()->ToBattlegroundMap();
        if (!bgMap)
            return;

        std::vector<Creature*> bots = CollectBotsOnTeam(bgMap, player->GetTeam());
        if (!bots.empty())
            FSBParty::SendBattlegroundRaidUpdate(player, bots);
    }

    void PeriodicRaidUpdate(BattlegroundMap* battlegroundMap)
    {
        if (!battlegroundMap)
            return;

        Battleground* bg = battlegroundMap->GetBG();
        if (!bg)
            return;

        for (auto const& [guid, _] : bg->GetPlayers())
        {
            Player* player = ObjectAccessor::GetPlayer(battlegroundMap, guid);
            if (!player)
                continue;

            if (!player->GetSession() || !player->IsInWorld())
                continue;

            if (player->IsBeingTeleportedNear() || player->IsBeingTeleportedFar() || player->IsBeingTeleported())
                continue;

            if (player->GetSession()->PlayerLoading())
                continue;

            std::vector<Creature*> bots = CollectBotsOnTeam(battlegroundMap, player->GetTeam());
            if (bots.empty())
                continue;

            FSBParty::SendBattlegroundRaidUpdate(player, bots);

            for (Creature* bot : bots)
                FSBParty::SendBotMemberState(player, bot);
        }
    }

    Unit* FindHostileTargetInBattleground(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return nullptr;

        BattlegroundMap* bgMap = bot->GetMap()->ToBattlegroundMap();
        if (!bgMap)
            return nullptr;

        Battleground* bg = bgMap->GetBG();
        if (!bg || bg->GetStatus() != STATUS_IN_PROGRESS)
            return nullptr;

        FSB_Race botRace = FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry());
        Team botTeam = FSBUtils::GetTeamFromFSBRace(botRace);
        if (botTeam != ALLIANCE && botTeam != HORDE)
            return nullptr;

        constexpr float ScanRange = 50.0f;

        uint32 bgTypeId = bg->GetTypeID();
        bool isCtf = (bgTypeId == BATTLEGROUND_WS || bgTypeId == BATTLEGROUND_WG_CTF);

        struct Candidate
        {
            Unit* target;
            bool isFlagCarrier;
            float healthPct;
            float distance;
        };

        std::vector<Candidate> candidates;

        auto isValidTarget = [&](Unit* target) -> bool
        {
            if (!target || !target->IsAlive() || !target->IsInWorld() || target->IsDuringRemoveFromWorld())
                return false;

            if (!bot->IsWithinDistInMap(target, ScanRange))
                return false;

            if (!bot->IsValidAttackTarget(target))
                return false;

            if (target->HasBreakableByDamageCrowdControlAura())
                return false;

            if (target->HasUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC) ||
                target->HasUnitFlag(UNIT_FLAG_UNINTERACTIBLE) ||
                target->HasUnitFlag(UNIT_FLAG_PACIFIED) ||
                target->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE))
                return false;

            return true;
        };

        // Hostile players
        for (auto const& [guid, bgPlayer] : bg->GetPlayers())
        {
            if (bgPlayer.Team == botTeam)
                continue;

            Player* player = ObjectAccessor::GetPlayer(bgMap, guid);
            if (!player)
                continue;

            if (!isValidTarget(player))
                continue;

            bool isFlagCarrier = false;
            if (isCtf)
                isFlagCarrier = player->HasAura(WarsongGulch::Spells::WarsongFlag) ||
                                player->HasAura(WarsongGulch::Spells::SilverwingFlag) ||
                                player->HasAura(WarsongGulch::Spells::AllianceFlag) ||
                                    player->HasAura(WarsongGulch::Spells::HordeFlag);

            candidates.push_back({ player, isFlagCarrier, player->GetHealthPct(), bot->GetDistance(player) });
        }

        // Hostile bots
        for (auto const& [guid, creature] : bgMap->GetObjectsStore().Data.Head)
        {
            if (!creature || !creature->IsBot())
                continue;

            Team creatureTeam = FSBUtils::GetTeamFromFSBRace(FSBMgr::Get()->GetBotRaceForEntry(creature->GetEntry()));
            if (creatureTeam == botTeam)
                continue;

            if (!isValidTarget(creature))
                continue;

            bool isFlagCarrier = false;
            if (isCtf)
                isFlagCarrier = creature->HasAura(WarsongGulch::Spells::WarsongFlag) ||
                                creature->HasAura(WarsongGulch::Spells::SilverwingFlag) ||
                                creature->HasAura(WarsongGulch::Spells::AllianceFlag) ||
                                creature->HasAura(WarsongGulch::Spells::HordeFlag);

            candidates.push_back({ creature, isFlagCarrier, creature->GetHealthPct(), bot->GetDistance(creature) });
        }

        if (candidates.empty())
            return nullptr;

        auto best = std::min_element(candidates.begin(), candidates.end(),
            [](Candidate const& a, Candidate const& b)
        {
            if (a.isFlagCarrier != b.isFlagCarrier)
                return a.isFlagCarrier;
            if (a.healthPct != b.healthPct)
                return a.healthPct < b.healthPct;
            return a.distance < b.distance;
        });

        TC_LOG_DEBUG("scripts.fsb.combat", "FSBBattleground::FindHostileTargetInBattleground: Bot {} selected target {} (flagCarrier={}, healthPct={}, distance={})",
            bot->GetName(), best->target->GetName(), best->isFlagCarrier, best->healthPct, best->distance);

        return best->target;
    }
}
