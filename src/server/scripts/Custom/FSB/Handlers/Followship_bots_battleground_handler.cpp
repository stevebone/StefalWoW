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

#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "Creature.h"
#include "DBCEnums.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Random.h"
#include "TemporarySummon.h"

namespace
{
    constexpr uint32 FSB_BG_BOT_COUNT = 3;
    constexpr uint32 FSB_BG_BOT_ENTRIES[FSB_BG_BOT_COUNT] =
    {
        911,  // Warrior
        198,  // Mage
        925   // Paladin
    };

    void SpawnEnemyBots(Battleground* battleground)
    {
        uint32 const allianceCount = battleground->GetPlayersCountByTeam(ALLIANCE);
        uint32 const hordeCount = battleground->GetPlayersCountByTeam(HORDE);

        if (allianceCount > 0 && hordeCount > 0)
            return;

        Team const enemyTeam = allianceCount > 0 ? HORDE : ALLIANCE;
        TeamId const enemyTeamId = Battleground::GetTeamIndexByTeamId(enemyTeam);
        WorldSafeLocsEntry const* startPos = battleground->GetTeamStartPosition(enemyTeamId);
        if (!startPos)
        {
            TC_LOG_ERROR("scripts.fsb.battleground", "FSB BG Handler: No start position for team {}", enemyTeam);
            return;
        }

        Player* summoner = nullptr;
        for (auto const& [guid, playerData] : battleground->GetPlayers())
        {
            if (Player* player = ObjectAccessor::FindPlayer(guid))
            {
                summoner = player;
                break;
            }
        }

        if (!summoner)
        {
            TC_LOG_ERROR("scripts.fsb.battleground", "FSB BG Handler: No player found to summon bots");
            return;
        }

        for (uint32 i = 0; i < FSB_BG_BOT_COUNT; ++i)
        {
            Position pos(startPos->Loc);
            pos.m_positionX += frand(-3.0f, 3.0f);
            pos.m_positionY += frand(-3.0f, 3.0f);

            TempSummon* summon = summoner->SummonCreature(FSB_BG_BOT_ENTRIES[i], pos, TEMPSUMMON_MANUAL_DESPAWN);
            if (!summon)
            {
                TC_LOG_ERROR("scripts.fsb.battleground", "FSB BG Handler: Failed to summon bot entry {}", FSB_BG_BOT_ENTRIES[i]);
                continue;
            }

            Creature* bot = static_cast<Creature*>(summon);
            bot->SetFaction(enemyTeam == ALLIANCE ? FACTION_ALLIANCE_GENERIC : FACTION_HORDE_GENERIC);
            bot->SetLevel(summoner->GetLevel());
            bot->SetFullHealth();
            bot->SetPvP(true);

            TC_LOG_DEBUG("scripts.fsb.battleground", "FSB BG Handler: Spawned enemy bot {} for team {}", bot->GetEntry(), enemyTeam);
        }

        TC_LOG_INFO("scripts.fsb.battleground", "FSB BG Handler: Spawned {} enemy bots for team {}", FSB_BG_BOT_COUNT, enemyTeam);
    }
}

namespace FSB
{
    void HandleBattlegroundStart(Battleground* battleground, BattlegroundMap* /*battlegroundMap*/)
    {
        if (!sBattlegroundMgr->IsFSBOverrideEnabled())
            return;

        SpawnEnemyBots(battleground);
    }
}

void AddSC_followship_bots_battleground_handler()
{
    // Registered via the existing Warsong Gulch battleground script hook.
}
