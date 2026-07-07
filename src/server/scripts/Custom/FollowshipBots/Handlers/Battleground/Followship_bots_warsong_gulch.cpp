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
#include <string>
#include <vector>

#include "Followship_bots_battleground_handler.h"
#include "Followship_bots_chatter_handler.h"
#include "Followship_bots_chat_handler.h"
#include "Followship_bots_events_handler.h"
#include "Followship_bots_recovery_handler.h"

namespace FSBBattleground::WarsongGulch
{
    std::array<std::string, 20> const SpawnChatLines = {{
        "Hello everyone!",
        "How is everyone today?",
        "Ready to kick some [enemyTeam] ass?",
        "I am gonna rest for a bit while you lot take care of the [enemyTeam].",
        "For the [ownTeam]!!!!",
        "Let's get this win for the [ownTeam]!",
        "Time to show the [enemyTeam] what we're made of.",
        "Good luck everyone, let's make the [ownTeam] proud.",
        "I'll do my part, you do yours.",
        "Another day, another battlefield.",
        "Let's crush the [enemyTeam] and go home.",
        "The [enemyTeam] won't know what hit them.",
        "Stay focused, fight for the [ownTeam].",
        "I'm here to help secure the victory.",
        "No mercy for the [enemyTeam] today.",
        "Let's work together and win this.",
        "The [ownTeam] will prevail!",
        "I'm ready when you are.",
        "Don't let the [enemyTeam] get comfortable.",
        "Victory belongs to the [ownTeam]!"
    }};

    std::string FormatChatLine(std::string const& line, Team botTeam)
    {
        std::string result = line;
        std::string own = botTeam == ALLIANCE ? "Alliance" : "Horde";
        std::string enemy = botTeam == ALLIANCE ? "Horde" : "Alliance";
        FSBChatter::ReplaceAll(result, "[ownTeam]", own);
        FSBChatter::ReplaceAll(result, "[enemyTeam]", enemy);
        return result;
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

    Team GetExitTeam(Creature* bot, FSB_BattlegroundData* bgData)
    {
        Team botTeam = GetBotTeam(bot);
        return bgData->wsgState == WSGState::ReturnFlag ? GetEnemyTeam(botTeam) : botTeam;
    }

    Team GetAttackTargetTeam(Creature* bot, FSB_BattlegroundData* bgData)
    {
        Team botTeam = GetBotTeam(bot);
        if (bgData->wsgState == WSGState::AttackFlag)
            return GetEnemyTeam(botTeam);
        return botTeam; // ReturnFlag, DefendBase, ProtectCarrier
    }

    uint8 GetClosestPathStep(Creature* bot, WSGPath const& path, uint8 fromStep)
    {
        if (!bot || path.count == 0)
            return 0;

        if (fromStep >= path.count)
            return path.count - 1;

        Position botPos = bot->GetPosition();
        uint8 bestStep = fromStep;
        float bestDistSq = botPos.GetExactDistSq(path.points[fromStep]);

        for (uint8 i = fromStep + 1; i < path.count; ++i)
        {
            float distSq = botPos.GetExactDistSq(path.points[i]);
            if (distSq < bestDistSq)
            {
                bestDistSq = distSq;
                bestStep = i;
            }
        }

        return bestStep;
    }

    void MoveToExitStep(Creature* bot, FSB_BattlegroundData* bgData)
    {
        WSGPath const& path = GetExitPath(GetExitTeam(bot, bgData), bgData->wsgExitPathChoice);
        bgData->wsgPathStep = GetClosestPathStep(bot, path, bgData->wsgPathStep);
        uint8 step = std::min<uint8>(bgData->wsgPathStep, path.count - 1);
        bot->GetMotionMaster()->MovePoint(FSBMovement::MOVEMENT_POINT_WSG_EXIT, path.points[step]);
    }

    void MoveToAttackStep(Creature* bot, FSB_BattlegroundData* bgData)
    {
        WSGPath const& path = GetAttackPathTowards(GetAttackTargetTeam(bot, bgData), bgData->wsgAttackPathChoice);
        bgData->wsgPathStep = GetClosestPathStep(bot, path, bgData->wsgPathStep);
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

                std::string spawnMsg = FormatChatLine(SpawnChatLines[urand(0, SpawnChatLines.size() - 1)], team);
                FSBEvents::ScheduleBotEventWithChatter(bot, FSB_EVENT_WSG_SPAWN_CHAT, 3s, 15s, FSB_ReplyType::Raid, spawnMsg, nullptr);

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

    Creature* FindFriendlyCarrier(Creature* bot)
    {
        Team botTeam = GetBotTeam(bot);
        BattlegroundMap* bgMap = bot->GetMap()->ToBattlegroundMap();
        if (!bgMap)
            return nullptr;

        for (auto const& [guid, creature] : bgMap->GetObjectsStore().Data.Head)
        {
            if (!creature || !creature->IsBot() || creature == bot)
                continue;

            Team creatureTeam = FSBUtils::GetTeamFromFSBRace(FSBMgr::Get()->GetBotRaceForEntry(creature->GetEntry()));
            if (creatureTeam != botTeam)
                continue;

            if (BotHasFlagAura(creature))
                return creature;
        }
        return nullptr;
    }

    void UpdateBot(Creature* bot, FSB_BattlegroundData* bgData)
    {
        if (!bot || !bgData)
            return;

        if (!bot->IsAlive())
            return;

        if (bot->IsInCombat() && (bgData->wsgState != WSGState::ReturnFlag || bgData->wsgState != WSGState::AttackFlag))
            return;

        if (FSBRecovery::BotHasRecoveryActive(bot))
            return;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (baseAI && baseAI->botHired)
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

        // If a friendly bot is carrying the flag, AttackFlag bots either protect it
        // or fall back to the center depending on distance.
        if (bgData->wsgState == WSGState::AttackFlag)
        {
            if (Creature* carrier = FindFriendlyCarrier(bot))
            {
                if (bot->GetDistance(carrier) < 50.0f)
                    SetBotState(bot, bgData, WSGState::ProtectCarrier);
                else
                    SetBotState(bot, bgData, WSGState::HoldCenter);
                return;
            }
        }

        switch (bgData->wsgState)
        {
        case WSGState::DefendBase:
        {
            Position const& basePos = botTeam == ALLIANCE ? FSB_WSG_BASE_ALLIANCE : FSB_WSG_BASE_HORDE;
            if (bgData->wsgMovePhase == WSGMovePhase::None && bot->GetDistance(basePos) > 50.0f)
            {
                // Far from base (e.g., re-rolling from center): use the path home.
                bgData->wsgMovePhase = WSGMovePhase::Attacking;
                bgData->wsgPathStep = 0;
                MoveToAttackStep(bot, bgData);
            }
            else if (bgData->wsgMovePhase == WSGMovePhase::Attacking)
            {
                MoveToAttackStep(bot, bgData);
            }
            else
            {
                // Near the base: shuffle around the flag room.
                Position movePos = GetPositionWithOffsetForState(WSGState::DefendBase, basePos);
                bot->GetMotionMaster()->MovePoint(FSBMovement::MOVEMENT_POINT_WSG_DEFEND, movePos);
            }
            break;
        }
        case WSGState::ProtectCarrier:
        {
            Creature* carrier = FindFriendlyCarrier(bot);
            if (!carrier || bot->GetDistance(carrier) > 50.0f)
            {
                SetBotState(bot, bgData, WSGState::HoldCenter);
                break;
            }

            Position target = GetRandomOffsetPosition(carrier->GetPosition(), 5.0f, 12.0f);
            bot->GetMotionMaster()->MovePoint(FSBMovement::MOVEMENT_POINT_WSG_DEFEND, target);
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

        // Update tracked state counts via ZoneScript GetData/SetData (per team)
        if (ZoneScript* zoneScript = bot->GetZoneScript())
        {
            bool isAlliance = GetBotTeam(bot) == ALLIANCE;
            uint32 baseDefendersId = isAlliance ? FSBBattleground::DataCurrentBaseDefendersAlliance : FSBBattleground::DataCurrentBaseDefendersHorde;
            uint32 middleDefendersId = isAlliance ? FSBBattleground::DataCurrentMiddleDefendersAlliance : FSBBattleground::DataCurrentMiddleDefendersHorde;
            uint32 flagAttackersId = isAlliance ? FSBBattleground::DataCurrentFlagAttackersAlliance : FSBBattleground::DataCurrentFlagAttackersHorde;

            WSGState oldState = bgData->wsgState;

            switch (oldState)
            {
                case WSGState::DefendBase:
                    zoneScript->SetData(baseDefendersId, zoneScript->GetData(baseDefendersId) - 1);
                    break;
                case WSGState::HoldCenter:
                    zoneScript->SetData(middleDefendersId, zoneScript->GetData(middleDefendersId) - 1);
                    break;
                case WSGState::AttackFlag:
                    zoneScript->SetData(flagAttackersId, zoneScript->GetData(flagAttackersId) - 1);
                    break;
                default:
                    break;
            }

            switch (newState)
            {
                case WSGState::DefendBase:
                    zoneScript->SetData(baseDefendersId, zoneScript->GetData(baseDefendersId) + 1);
                    break;
                case WSGState::HoldCenter:
                    zoneScript->SetData(middleDefendersId, zoneScript->GetData(middleDefendersId) + 1);
                    break;
                case WSGState::AttackFlag:
                    zoneScript->SetData(flagAttackersId, zoneScript->GetData(flagAttackersId) + 1);
                    break;
                default:
                    break;
            }
        }

        bgData->wsgState = newState;
        bgData->wsgMovePhase = WSGMovePhase::None;
        bgData->wsgPathStep = 0;
        bgData->wsgExitPathChoice = static_cast<WSGExitPathChoice>(urand(1, 2));
        bgData->wsgAttackPathChoice = static_cast<WSGExitPathChoice>(urand(1, 2));

        Team botTeam = GetBotTeam(bot);
        if (botTeam != ALLIANCE && botTeam != HORDE)
            return;

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
        case WSGState::ProtectCarrier:
        {
            static char const* const messages[] = { "I'll protect the flag carrier!", "Covering the runner!", "Stay close to the carrier!" };
            msg = messages[urand(0, 2)];
            break;
        }
        default:
            break;
        }

        if (msg)
        {
            std::string formatted = FormatChatLine(msg, botTeam);
            FSBEvents::ScheduleBotEventWithChatter(bot, FSB_EVENT_WSG_STATE_CHAT, 2s, 10s, FSB_ReplyType::Raid, formatted, nullptr);
        }
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
                if (bgData->wsgState == WSGState::DefendBase)
                {
                    // Returning home to defend: move to the base position so the tick can shuffle.
                    bgData->wsgMovePhase = WSGMovePhase::None;
                    Position const& basePos = GetBotTeam(bot) == ALLIANCE ? FSB_WSG_BASE_ALLIANCE : FSB_WSG_BASE_HORDE;
                    bot->GetMotionMaster()->MovePoint(FSBMovement::MOVEMENT_POINT_WSG_DEFEND, basePos);
                }
                else
                {
                    bgData->wsgMovePhase = WSGMovePhase::MovingToFlag;
                    MoveToFlagPoint(bot, bgData);
                }
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

    std::vector<uint32> GetPvpStatIds()
    {
        return { 928, 929 }; // PVP_STAT_FLAG_CAPTURES, PVP_STAT_FLAG_RETURNS
    }

    bool IsEnemyFlagTaken(Creature* bot)
    {
        if (!bot)
            return false;

        Team botTeam = GetBotTeam(bot);
        uint32 enemyFlagEntry = botTeam == ALLIANCE ? ObjectHordeFlag : ObjectAllianceFlag;

        BattlegroundMap* bgMap = bot->GetMap()->ToBattlegroundMap();
        if (!bgMap)
            return false;

        bool found = false;
        for (auto const& [guid, go] : bgMap->GetObjectsStore().Data.FindContainer<GameObject>())
        {
            if (!go || go->GetEntry() != enemyFlagEntry)
                continue;

            found = true;
            TC_LOG_DEBUG("scripts.fsb.battleground", "FSB WSG: IsEnemyFlagTaken bot {} found enemy flag {} state={}", bot->GetName(), enemyFlagEntry, static_cast<uint8>(go->GetFlagState()));
            return go->GetFlagState() == FlagState::Taken;
        }

        if (!found)
            TC_LOG_DEBUG("scripts.fsb.battleground", "FSB WSG: IsEnemyFlagTaken bot {} found no gameobject with entry {}", bot->GetName(), enemyFlagEntry);

        return false;
    }

    WSGState GetWSGBotState(Creature* bot, WSGState /*currentState*/)
    {
        bool enemyFlagTaken = IsEnemyFlagTaken(bot);

        ZoneScript* zoneScript = bot ? bot->GetZoneScript() : nullptr;

        std::vector<WSGState> available;
        if (zoneScript)
        {
            bool isAlliance = GetBotTeam(bot) == ALLIANCE;
            uint32 baseDefendersId = isAlliance ? FSBBattleground::DataCurrentBaseDefendersAlliance : FSBBattleground::DataCurrentBaseDefendersHorde;
            uint32 middleDefendersId = isAlliance ? FSBBattleground::DataCurrentMiddleDefendersAlliance : FSBBattleground::DataCurrentMiddleDefendersHorde;
            uint32 flagAttackersId = isAlliance ? FSBBattleground::DataCurrentFlagAttackersAlliance : FSBBattleground::DataCurrentFlagAttackersHorde;

            if (zoneScript->GetData(baseDefendersId) < MaxBaseDefenders)
                available.push_back(WSGState::DefendBase);
            if (zoneScript->GetData(middleDefendersId) < MaxMiddleDefenders)
                available.push_back(WSGState::HoldCenter);
            if (!enemyFlagTaken && zoneScript->GetData(flagAttackersId) < MaxFlagAttackers)
                available.push_back(WSGState::AttackFlag);
        }
        else
        {
            available.push_back(WSGState::DefendBase);
            available.push_back(WSGState::HoldCenter);
            if (!enemyFlagTaken)
                available.push_back(WSGState::AttackFlag);
        }

        if (available.empty())
            return WSGState::HoldCenter;

        return available[urand(0, available.size() - 1)];
    }

    void OnBotCapturedFlag(Creature* bot)
    {
        if (!bot || !bot->IsBot())
            return;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (baseAI && baseAI->botHired)
            return;

        // Re-roll the capturing bot after a short delay.
        FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_WSG_REROLL_STATE, 10s);

        // Re-roll all non-hired WSG bots currently holding center.
        BattlegroundMap* bgMap = bot->GetMap()->ToBattlegroundMap();
        if (!bgMap)
            return;

        for (auto const& [guid, creature] : bgMap->GetObjectsStore().Data.Head)
        {
            if (!creature || !creature->IsBot() || creature == bot)
                continue;

            auto otherAI = dynamic_cast<FSB_BaseAI*>(creature->AI());
            if (!otherAI || otherAI->botHired)
                continue;

            FSB_BattlegroundData* otherBgData = otherAI->GetBattlegroundData();
            if (!otherBgData || (otherBgData->bgTypeId != BATTLEGROUND_WS && otherBgData->bgTypeId != BATTLEGROUND_WG_CTF))
                continue;

            if (otherBgData->wsgState == WSGState::HoldCenter)
                FSBEvents::ScheduleBotEvent(creature, FSB_EVENT_WSG_REROLL_STATE, 10s);
        }
    }
}
