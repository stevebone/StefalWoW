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
#include "Creature.h"
#include "DBCEnums.h"
#include "Followship_bots_battleground_handler.h"
#include "Followship_bots_chat_handler.h"
#include "Followship_bots_events_handler.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_movement_handler.h"
#include "Followship_bots_utils.h"
#include "GameObject.h"
#include "Log.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Random.h"

#include <algorithm>
#include <array>
#include <vector>

namespace FSBBattleground::WarsongGulch
{
    constexpr uint32 FSB_WSG_MAX_TEAM_SIZE = 10;

    namespace
    {
        constexpr float FSB_WSG_JUMP_SPEED = 15.0f;
        constexpr float FSB_WSG_JUMP_MAX_HEIGHT = 8.0f;
        constexpr float FSB_WSG_CENTER_OFFSET_RADIUS = 8.0f;
        constexpr float FSB_WSG_FLAG_SEARCH_RANGE = 10.0f;

        struct WSGPath
        {
            std::array<Position, 6> points;
            uint8 count;
            bool hasJump;
            Position jump;
        };

        WSGPath const AllianceExitPath1 =
        {
            { Position(1505.3951f, 1492.0402f, 352.3743f), Position(1455.5461f, 1493.4531f, 351.6170f), Position(1422.3452f, 1530.2088f, 340.6996f) },
            3, true, Position(1400.0762f, 1531.7379f, 322.7053f)
        };

        WSGPath const AllianceExitPath2 =
        {
            { Position(1509.0966f, 1457.5687f, 350.6935f), Position(1441.1824f, 1460.2907f, 341.5768f), Position(1363.4025f, 1461.5205f, 324.7423f) },
            3, true, Position(1350.3236f, 1461.3178f, 323.6935f)
        };

        WSGPath const HordeExitPath1 =
        {
            { Position(939.0695f, 1459.5577f, 344.7154f), Position(1011.6952f, 1459.5695f, 334.4982f), Position(1117.2037f, 1462.1458f, 316.4730f) },
            3, true, Position(1131.3631f, 1460.2930f, 314.6424f)
        };

        WSGPath const HordeExitPath2 =
        {
            { Position(947.0158f, 1422.6073f, 345.4355f), Position(995.0747f, 1422.5394f, 345.1290f), Position(1062.1445f, 1400.8360f, 336.7558f) },
            3, true, Position(1075.2363f, 1396.4595f, 322.2837f)
        };

        // "X Attack Path" leads toward X's base (used by the enemy of X to attack, or by X to return home).
        WSGPath const AllianceAttackPath1 =
        {
            {
                Position(1363.4025f, 1461.5205f, 324.7423f),
                Position(1441.1824f, 1460.2907f, 341.5768f),
                Position(1509.0966f, 1457.5687f, 350.6935f)
            },
            3, false, Position()
        };

        WSGPath const AllianceAttackPath2 =
        {
            { Position(1349.7447f, 1407.4890f, 325.2285f), Position(1372.4014f, 1379.3076f, 330.2068f), Position(1408.5594f, 1408.8072f, 344.3476f),
              Position(1407.9438f, 1460.0124f, 348.0763f), Position(1467.9635f, 1458.8614f, 362.9517f), Position(1529.7808f, 1460.1724f, 362.7264f) },
            6, true, Position(1527.6835f, 1471.8970f, 351.9594f)
        };

        WSGPath const HordeAttackPath1 =
        {
            { Position(1117.2037f, 1462.1458f, 316.4730f), Position(1011.6952f, 1459.5695f, 334.4982f), Position(939.0695f, 1459.5577f, 344.7154f) },
            3, false, Position()
        };

        WSGPath const HordeAttackPath2 =
        {
            { Position(1091.7185f, 1535.4669f, 315.9850f), Position(1066.9182f, 1554.7525f, 321.3725f), Position(1042.7249f, 1530.6914f, 336.6773f),
              Position(1057.0529f, 1455.6038f, 341.5280f), Position(985.4289f, 1458.1486f, 356.3080f), Position(927.8468f, 1457.0494f, 356.0725f) },
            6, true, Position(925.6894f, 1445.4456f, 345.5816f)
        };

        std::array<Position, 4> const CenterPositions =
        {
            Position(1209.4315f, 1520.9934f, 309.0975f, 5.0f),
            Position(1214.3841f, 1480.7662f, 307.9321f, 3.5f),
            Position(1226.8264f, 1408.7434f, 310.8634f, 3.0f),
            Position(1240.8289f, 1339.2724f, 312.3503f, 2.32f)
        };

        Team GetBotTeam(Creature* bot)
        {
            return FSBUtils::GetTeamFromFSBRace(FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry()));
        }

        Team GetEnemyTeam(Team team)
        {
            return team == ALLIANCE ? HORDE : ALLIANCE;
        }

        WSGPath const& GetExitPath(Team team, WSGExitPathChoice choice)
        {
            if (team == ALLIANCE)
                return choice == WSGExitPathChoice::Path1 ? AllianceExitPath1 : AllianceExitPath2;
            return choice == WSGExitPathChoice::Path1 ? HordeExitPath1 : HordeExitPath2;
        }

        // Returns the path leading toward targetTeam's base.
        WSGPath const& GetAttackPathTowards(Team targetTeam, WSGExitPathChoice choice)
        {
            if (targetTeam == ALLIANCE)
                return choice == WSGExitPathChoice::Path1 ? AllianceAttackPath1 : AllianceAttackPath2;
            return choice == WSGExitPathChoice::Path1 ? HordeAttackPath1 : HordeAttackPath2;
        }

        // Exit paths start at the bot's own base, except during ReturnFlag where the bot exits the enemy base.
        Team GetExitTeam(Creature* bot, FSB_BattlegroundData* bgData)
        {
            Team botTeam = GetBotTeam(bot);
            return bgData->wsgState == WSGState::ReturnFlag ? GetEnemyTeam(botTeam) : botTeam;
        }

        // Attack paths lead toward the enemy base during AttackFlag, or back home during ReturnFlag.
        Team GetAttackTargetTeam(Creature* bot, FSB_BattlegroundData* bgData)
        {
            Team botTeam = GetBotTeam(bot);
            return bgData->wsgState == WSGState::AttackFlag ? GetEnemyTeam(botTeam) : botTeam;
        }

        Position GetRandomOffsetPosition(Position const& basePos, float minRadius, float maxRadius)
        {
            float angle = frand(0.0f, 6.283185307f);
            float distance = frand(minRadius, maxRadius);
            Position offset(std::cos(angle) * distance, std::sin(angle) * distance, 0.0f, 0.0f);
            return basePos.GetPositionWithOffset(offset);
        }

        void MoveToExitStep(Creature* bot, FSB_BattlegroundData* bgData)
        {
            WSGPath const& path = GetExitPath(GetExitTeam(bot, bgData), bgData->wsgExitPathChoice);
            uint8 step = std::min<uint8>(bgData->wsgPathStep, path.count - 1);
            bot->GetMotionMaster()->MovePoint(FSBMovement::MOVEMENT_POINT_WSG_EXIT, path.points[step]);
        }

        void MoveToAttackStep(Creature* bot, FSB_BattlegroundData* bgData)
        {
            WSGPath const& path = GetAttackPathTowards(GetAttackTargetTeam(bot, bgData), bgData->wsgAttackPathChoice);
            uint8 step = std::min<uint8>(bgData->wsgPathStep, path.count - 1);
            bot->GetMotionMaster()->MovePoint(FSBMovement::MOVEMENT_POINT_WSG_ATTACK, path.points[step]);
        }

        void MoveToCenter(Creature* bot, FSB_BattlegroundData* bgData, bool withOffset, bool pickNew)
        {
            if (pickNew)
                bgData->wsgCenterIndex = urand(0, CenterPositions.size() - 1);

            Position target = CenterPositions[bgData->wsgCenterIndex];
            if (withOffset)
                target = GetRandomOffsetPosition(target, 0.0f, FSB_WSG_CENTER_OFFSET_RADIUS);

            bot->GetMotionMaster()->MovePoint(FSBMovement::MOVEMENT_POINT_WSG_CENTER, target);
        }

        void MoveToFlagPoint(Creature* bot, FSB_BattlegroundData* bgData)
        {
            Team botTeam = GetBotTeam(bot);
            if (bgData->wsgState == WSGState::AttackFlag)
            {
                Position const& enemyFlag = botTeam == ALLIANCE ? FSB_WSG_FLAG_HORDE : FSB_WSG_FLAG_ALLIANCE;
                bot->GetMotionMaster()->MovePoint(FSBMovement::MOVEMENT_POINT_WSG_OBJECTIVE, enemyFlag);
            }
            else // ReturnFlag
            {
                Position const& ownFlag = botTeam == ALLIANCE ? FSB_WSG_FLAG_ALLIANCE : FSB_WSG_FLAG_HORDE;
                bot->GetMotionMaster()->MovePoint(FSBMovement::MOVEMENT_POINT_WSG_RETURN_FLAG, ownFlag);
            }
        }

        bool BotHasFlagAura(Creature* bot)
        {
            return bot->HasAura(Spells::WarsongFlag) || bot->HasAura(Spells::SilverwingFlag)
                || bot->HasAura(Spells::AllianceFlag) || bot->HasAura(Spells::HordeFlag);
        }
    }

    Position GetPositionWithOffsetForState(WSGState state, Position const& basePos)
    {
        float minRadius = 0.0f;
        float maxRadius = 0.0f;

        switch (state)
        {
        case WSGState::DefendBase:
            minRadius = 3.0f;
            maxRadius = 5.0f;
            break;
        case WSGState::HoldCenter:
            minRadius = 5.0f;
            maxRadius = 10.0f;
            break;
        case WSGState::AttackFlag:
        default:
            return basePos;
        }

        float angle = frand(0.0f, 6.283185307f);
        float distance = frand(minRadius, maxRadius);
        Position offset(std::cos(angle) * distance, std::sin(angle) * distance, 0.0f, 0.0f);
        return basePos.GetPositionWithOffset(offset);
    }

    void SpawnBots(Battleground* battleground, Player* triggeringPlayer)
    {
        TC_LOG_INFO("scripts.fsb.battleground", "FSB WSG Handler: SpawnBots entered");

        FSBBattleground::ClearSpawnedBotGuids(battleground->GetBgMap());

        // Count real players from GetPlayers() instead of GetPlayersCountByTeam(),
        // because the triggering player may not yet be registered in the BG's team count.
        uint32 alliancePlayers = 0;
        uint32 hordePlayers = 0;
        bool triggeringInRoster = false;
        for (auto const& [guid, bgPlayer] : battleground->GetPlayers())
        {
            if (triggeringPlayer && guid == triggeringPlayer->GetGUID())
                triggeringInRoster = true;

            if (bgPlayer.Team == ALLIANCE)
                ++alliancePlayers;
            else if (bgPlayer.Team == HORDE)
                ++hordePlayers;
        }

        // If the triggering player is not yet in the BG roster, account for them.
        if (triggeringPlayer && !triggeringInRoster)
        {
            if (triggeringPlayer->GetTeam() == ALLIANCE)
                ++alliancePlayers;
            else if (triggeringPlayer->GetTeam() == HORDE)
                ++hordePlayers;
        }

        uint32 const allianceExistingBots = FSBBattleground::CountExistingBots(battleground, ALLIANCE);
        uint32 const hordeExistingBots = FSBBattleground::CountExistingBots(battleground, HORDE);

        int32 const allianceNeeded = std::max(0, int32(FSB_WSG_MAX_TEAM_SIZE) - int32(alliancePlayers) - int32(allianceExistingBots));
        int32 const hordeNeeded = std::max(0, int32(FSB_WSG_MAX_TEAM_SIZE) - int32(hordePlayers) - int32(hordeExistingBots));

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

            Team team = FSBUtils::GetTeamFromFSBRace(templateData.botRace);
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

            std::vector<uint32> selected = FSBBattleground::SelectRandomEntries(availableEntries, static_cast<uint32>(needed));
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
                FSBBattleground::AddBotSpawnGuid(battleground->GetBgMap(), bot->GetGUID());

                TC_LOG_INFO("scripts.fsb.battleground", "FSB WSG Handler: Spawned bot {} for team {}", bot->GetEntry(), team);
            }

            TC_LOG_INFO("scripts.fsb.battleground", "FSB WSG Handler: Spawned bots for team {}", team);
        };

        spawnTeam(ALLIANCE, allianceNeeded, allianceEntries);
        spawnTeam(HORDE, hordeNeeded, hordeEntries);
    }

    void InitializeBot(Creature* bot, FSB_BattlegroundData* bgData, Battleground* bg)
    {
        if (!bot || !bgData || !bg)
            return;

        Milliseconds delay = FSBBattleground::IsInProgress(bot) ? 100ms : 2min;
        FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_BATTLEGROUND_START, delay);
    }

    void UpdateBot(Creature* bot, FSB_BattlegroundData* bgData)
    {
        if (!bot || !bgData)
            return;

        if (!bot->IsAlive())
            return;

        if (bot->IsInCombat())
            return;

        MovementGeneratorType moveType = FSBMovement::GetMovementType(bot);
        if (moveType == POINT_MOTION_TYPE || moveType == EFFECT_MOTION_TYPE)
            return;

        BattlegroundMap* bgMap = bot->GetMap()->ToBattlegroundMap();
        if (!bgMap)
            return;

        Battleground* bg = bgMap->GetBG();
        if (!bg)
            return;

        if (bg->GetStatus() != STATUS_IN_PROGRESS)
            return;

        Team botTeam = GetBotTeam(bot);
        if (botTeam != ALLIANCE && botTeam != HORDE)
            return;

        switch (bgData->wsgState)
        {
        case WSGState::DefendBase:
        {
            // Phase stays None; every tick re-shuffles the defensive position.
            Position const& basePos = botTeam == ALLIANCE ? FSB_WSG_BASE_ALLIANCE : FSB_WSG_BASE_HORDE;
            Position movePos = GetPositionWithOffsetForState(WSGState::DefendBase, basePos);
            bot->GetMotionMaster()->MovePoint(FSBMovement::MOVEMENT_POINT_WSG_DEFEND, movePos);
            break;
        }
        case WSGState::HoldCenter:
        case WSGState::AttackFlag:
        case WSGState::ReturnFlag:
        {
            switch (bgData->wsgMovePhase)
            {
            case WSGMovePhase::None:
                bgData->wsgMovePhase = WSGMovePhase::Exiting;
                bgData->wsgPathStep = 0;
                MoveToExitStep(bot, bgData);
                break;
            case WSGMovePhase::Exiting:
                // Combat or death interrupted the chain; resume the current exit step.
                MoveToExitStep(bot, bgData);
                break;
            case WSGMovePhase::MovingToCenter:
                MoveToCenter(bot, bgData, bgData->wsgState == WSGState::HoldCenter, false);
                break;
            case WSGMovePhase::Attacking:
                MoveToAttackStep(bot, bgData);
                break;
            case WSGMovePhase::MovingToFlag:
                MoveToFlagPoint(bot, bgData);
                break;
            case WSGMovePhase::AtObjective:
                // HoldCenter bots shuffle around their chosen center position.
                if (bgData->wsgState == WSGState::HoldCenter)
                    MoveToCenter(bot, bgData, true, false);
                break;
            default:
                break;
            }
            break;
        }
        default:
            break;
        }
    }

    void SetBotState(Creature* bot, FSB_BattlegroundData* bgData, WSGState newState)
    {
        if (!bot || !bgData)
            return;

        bgData->wsgState = newState;
        bgData->wsgMovePhase = WSGMovePhase::None;
        bgData->wsgPathStep = 0;
        bgData->wsgExitPathChoice = static_cast<WSGExitPathChoice>(urand(1, 2));
        bgData->wsgAttackPathChoice = static_cast<WSGExitPathChoice>(urand(1, 2));

        char const* msg = nullptr;
        switch (newState)
        {
        case WSGState::DefendBase:
        {
            static char const* const messages[] = { "I'll stay back and defend the flag room.", "Guarding our base.", "I'm on defense." };
            msg = messages[urand(0, 2)];
            break;
        }
        case WSGState::HoldCenter:
        {
            static char const* const messages[] = { "I'll hold mid.", "Controlling the center.", "Taking mid, watch the roads." };
            msg = messages[urand(0, 2)];
            break;
        }
        case WSGState::AttackFlag:
        {
            static char const* const messages[] = { "Going for their flag!", "I'll run the flag, cover me.", "Heading into their base!" };
            msg = messages[urand(0, 2)];
            break;
        }
        case WSGState::ReturnFlag:
        {
            static char const* const messages[] = { "Got the flag! Heading home!", "Flag secured - clear a path!", "I have their flag, cover my run!" };
            msg = messages[urand(0, 2)];
            break;
        }
        default:
            break;
        }

        if (msg)
            FSBChat::BotSendRaidChat(bot, msg);
    }

    void TryUseEnemyFlag(Creature* bot, FSB_BattlegroundData* bgData)
    {
        if (!bot || !bgData || !bot->IsAlive())
            return;

        if (bgData->wsgState != WSGState::AttackFlag)
            return;

        Team botTeam = GetBotTeam(bot);
        if (botTeam != ALLIANCE && botTeam != HORDE)
            return;

        uint32 flagEntry = botTeam == ALLIANCE ? ObjectHordeFlag : ObjectAllianceFlag;
        GameObject* flag = bot->FindNearestGameObject(flagEntry, FSB_WSG_FLAG_SEARCH_RANGE);
        if (!flag)
            return;

        // GameObject::Use is a no-op unless the flag state is InBase, so only the first bot picks it up.
        flag->Use(bot);

        if (BotHasFlagAura(bot))
        {
            SetBotState(bot, bgData, WSGState::ReturnFlag);
            UpdateBot(bot, bgData); // start the return journey immediately
        }
        // else: another bot picked it first; the next tick re-issues the objective move and retries.
    }

    void OnMovementInform(Creature* bot, FSB_BattlegroundData* bgData, uint32 type, uint32 id)
    {
        if (!bot || !bgData || !bot->IsAlive())
            return;

        if (type == EFFECT_MOTION_TYPE)
        {
            if (id == FSBMovement::MOVEMENT_POINT_WSG_JUMP)
            {
                // Exit path jump landed; head to a random center position.
                bgData->wsgMovePhase = WSGMovePhase::MovingToCenter;
                MoveToCenter(bot, bgData, bgData->wsgState == WSGState::HoldCenter, true);
            }
            else if (id == FSBMovement::MOVEMENT_POINT_WSG_ATTACK_JUMP)
            {
                // Attack path jump landed; head to the flag position.
                bgData->wsgMovePhase = WSGMovePhase::MovingToFlag;
                MoveToFlagPoint(bot, bgData);
            }
            return;
        }

        if (type != POINT_MOTION_TYPE)
            return;

        switch (id)
        {
        case FSBMovement::MOVEMENT_POINT_WSG_DEFEND:
            // No-op; the tick handles the re-shuffle.
            break;
        case FSBMovement::MOVEMENT_POINT_WSG_EXIT:
        {
            WSGPath const& path = GetExitPath(GetExitTeam(bot, bgData), bgData->wsgExitPathChoice);
            ++bgData->wsgPathStep;
            if (bgData->wsgPathStep < path.count)
                MoveToExitStep(bot, bgData);
            else if (path.hasJump)
                bot->GetMotionMaster()->MoveJump(FSBMovement::MOVEMENT_POINT_WSG_JUMP, path.jump, FSB_WSG_JUMP_SPEED, {}, FSB_WSG_JUMP_MAX_HEIGHT);
            else
            {
                bgData->wsgMovePhase = WSGMovePhase::MovingToCenter;
                MoveToCenter(bot, bgData, bgData->wsgState == WSGState::HoldCenter, true);
            }
            break;
        }
        case FSBMovement::MOVEMENT_POINT_WSG_CENTER:
        {
            if (bgData->wsgState == WSGState::HoldCenter)
            {
                bgData->wsgMovePhase = WSGMovePhase::AtObjective;
            }
            else if (bgData->wsgState == WSGState::AttackFlag || bgData->wsgState == WSGState::ReturnFlag)
            {
                bgData->wsgMovePhase = WSGMovePhase::Attacking;
                bgData->wsgPathStep = 0;
                MoveToAttackStep(bot, bgData);
            }
            break;
        }
        case FSBMovement::MOVEMENT_POINT_WSG_ATTACK:
        {
            WSGPath const& path = GetAttackPathTowards(GetAttackTargetTeam(bot, bgData), bgData->wsgAttackPathChoice);
            ++bgData->wsgPathStep;
            if (bgData->wsgPathStep < path.count)
                MoveToAttackStep(bot, bgData);
            else if (path.hasJump)
                bot->GetMotionMaster()->MoveJump(FSBMovement::MOVEMENT_POINT_WSG_ATTACK_JUMP, path.jump, FSB_WSG_JUMP_SPEED, {}, FSB_WSG_JUMP_MAX_HEIGHT);
            else
            {
                bgData->wsgMovePhase = WSGMovePhase::MovingToFlag;
                MoveToFlagPoint(bot, bgData);
            }
            break;
        }
        case FSBMovement::MOVEMENT_POINT_WSG_OBJECTIVE:
            // Reached the enemy flag position; use the flag after a 1s delay.
            FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_WSG_USE_FLAG, 1s);
            break;
        case FSBMovement::MOVEMENT_POINT_WSG_RETURN_FLAG:
            // Home with the flag; trigger the capture directly from the bot AI.
            bgData->wsgMovePhase = WSGMovePhase::AtObjective;
            if (ZoneScript* zoneScript = bot->GetZoneScript())
            {
                TC_LOG_DEBUG("scripts.fsb.battleground", "FSB WSG: bot {} reached return point, invoking capture event", bot->GetName());
                zoneScript->ProcessEvent(bot, FSB_EVENT_WSG_BOT_CAPTURE, bot);
            }
            break;
        default:
            break;
        }
    }
}
