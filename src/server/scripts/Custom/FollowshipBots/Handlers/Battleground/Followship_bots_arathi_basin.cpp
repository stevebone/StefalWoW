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

#include "Followship_bots_arathi_basin.h"

#include "Battleground.h"
#include "BattlegroundPackets.h"
#include "Creature.h"
#include "GameObject.h"
#include "Log.h"
#include "Map.h"
#include "MotionMaster.h"
#include "Random.h"

#include <vector>

#include "Followship_bots_ai_base.h"
#include "Followship_bots_battleground_handler.h"
#include "Followship_bots_events_handler.h"
#include "Followship_bots_movement_handler.h"

namespace FSBBattleground::ArathiBasin
{
    void SetBotState(Creature* bot, FSB_BattlegroundData* bgData, ABState newState)
    {
        if (!bot || !bgData)
            return;

        bgData->abState = newState;

        if (newState != ABState::None)
            FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_BATTLEGROUND_STATE_CHAT, 2s, 10s);
    }

    ABState GetABBotState(Creature* /*bot*/)
    {
        std::vector<ABState> available = {
            ABState::AttackStables,
            ABState::AttackMine,
            ABState::AttackMill,
            ABState::AttackBlacksmith,
            ABState::AttackFarm
        };

        return available[urand(0, available.size() - 1)];
    }

    GameObject* FindCapturePoint(Creature* bot, FSB_BattlegroundData* bgData)
    {
        BattlegroundMap* bgMap = bot->GetMap()->ToBattlegroundMap();
        if (!bgMap)
            return nullptr;

        uint32 entry = GetCapturePointEntryForState(bgData->abState);
        if (!entry)
            return nullptr;

        for (auto const& [guid, go] : bgMap->GetObjectsStore().Data.FindContainer<GameObject>())
        {
            if (go && go->GetEntry() == entry)
                return go;
        }

        return nullptr;
    }

    void TryAssaultCapturePoint(Creature* bot, FSB_BattlegroundData* bgData)
    {
        if (!bot || !bgData || !bot->IsAlive())
            return;

        GameObject* capturePoint = FindCapturePoint(bot, bgData);
        if (!capturePoint)
            return;

        if (!capturePoint->CanInteractWithCapturePoint(bot))
            return;

        TC_LOG_DEBUG("scripts.fsb.battleground", "FSB AB: Bot {} assaulting capture point {} state {}",
            bot->GetName(), capturePoint->GetEntry(), static_cast<uint32>(bgData->abState));

        capturePoint->AssaultCapturePoint(bot);
    }

    void CheckCapturePointState(Creature* bot, FSB_BattlegroundData* bgData)
    {
        if (!bot || !bgData || !bot->IsAlive())
            return;

        if (bgData->abState == ABState::None)
            return;

        auto isAttackState = [](ABState s)
        {
            return s == ABState::AttackStables || s == ABState::AttackMine || s == ABState::AttackMill
                || s == ABState::AttackBlacksmith || s == ABState::AttackFarm;
        };

        if (!isAttackState(bgData->abState))
            return;

        GameObject* capturePoint = FindCapturePoint(bot, bgData);
        if (!capturePoint)
            return;

        Team botTeam = FSBBattleground::GetBotTeam(bot);
        if (botTeam != ALLIANCE && botTeam != HORDE)
            return;

        auto const& state = capturePoint->GetGOValue()->CapturePoint.State;
        bool ownedByBotTeam = (botTeam == ALLIANCE && state == WorldPackets::Battleground::BattlegroundCapturePointState::AllianceCaptured)
            || (botTeam == HORDE && state == WorldPackets::Battleground::BattlegroundCapturePointState::HordeCaptured);

        if (ownedByBotTeam)
        {
            ABState defendState = GetDefendStateForAttack(bgData->abState);
            if (defendState != ABState::None)
            {
                TC_LOG_DEBUG("scripts.fsb.battleground", "FSB AB: Bot {} node captured, switching {} -> {}",
                    bot->GetName(), static_cast<uint32>(bgData->abState), static_cast<uint32>(defendState));
                SetBotState(bot, bgData, defendState);
            }
        }
    }

    void UpdateBot(Creature* bot, FSB_BattlegroundData* bgData)
    {
        if (!bot || !bgData || !bot->IsAlive())
            return;

        if (bgData->abState == ABState::None)
            return;

        BattlegroundMap* bgMap = bot->GetMap()->ToBattlegroundMap();
        if (!bgMap)
            return;

        Battleground* bg = bgMap->GetBG();
        if (!bg)
            return;

        if (bg->GetStatus() != STATUS_IN_PROGRESS)
            return;

        GameObject* capturePoint = FindCapturePoint(bot, bgData);
        if (!capturePoint)
        {
            TC_LOG_DEBUG("scripts.fsb.battleground", "FSB AB: UpdateBot bot {} state {} could not find capture point object",
                bot->GetName(), static_cast<uint32>(bgData->abState));
            return;
        }

        auto isAttackState = [](ABState s)
        {
            return s == ABState::AttackStables || s == ABState::AttackMine || s == ABState::AttackMill
                || s == ABState::AttackBlacksmith || s == ABState::AttackFarm;
        };

        // If in attack state and close enough to the capture point, assault it
        if (isAttackState(bgData->abState) && bot->GetDistance(capturePoint) < 10.0f)
        {
            TryAssaultCapturePoint(bot, bgData);
            return;
        }

        Position const& goPos = capturePoint->GetPosition();

        float angle = frand(0.0f, 6.283185307f);
        float offsetRadius = isAttackState(bgData->abState) ? 3.0f : 10.0f;
        Position offset(std::cos(angle) * offsetRadius, std::sin(angle) * offsetRadius, 0.0f, 0.0f);
        Position targetPos = goPos.GetPositionWithOffset(offset);

        uint32 movementPoint = isAttackState(bgData->abState)
            ? FSBMovement::MOVEMENT_POINT_AB_ATTACK_NODE
            : FSBMovement::MOVEMENT_POINT_AB_DEFEND_NODE;

        bot->GetMotionMaster()->MovePoint(movementPoint, targetPos, true);
    }

    void OnMovementInform(Creature* bot, FSB_BattlegroundData* bgData, uint32 /*type*/, uint32 id)
    {
        if (!bot || !bgData || !bot->IsAlive())
            return;

        if (id == FSBMovement::MOVEMENT_POINT_AB_ATTACK_NODE)
            return;

        if (id == FSBMovement::MOVEMENT_POINT_AB_DEFEND_NODE)
        {
            UpdateBot(bot, bgData);
            return;
        }
    }
}
