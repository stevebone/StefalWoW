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

#include "Position.h"
#include "SharedDefines.h"

class Battleground;
class Player;

struct FSB_BattlegroundData;

namespace FSBBattleground::WarsongGulch
{
    enum class WSGState : uint8
    {
        None,
        DefendBase,
        HoldCenter,
        AttackFlag,
        ReturnFlag
    };

    enum class WSGMovePhase : uint8
    {
        None,
        Exiting,
        MovingToCenter,
        Attacking,
        MovingToFlag,
        AtObjective
    };

    enum class WSGExitPathChoice : uint8
    {
        Path1 = 1,
        Path2 = 2
    };

    namespace Spells
    {
        static constexpr uint32 HordeFlag = 156618;
        static constexpr uint32 AllianceFlag = 156621;
        static constexpr uint32 WarsongFlag = 23333;
        static constexpr uint32 SilverwingFlag = 23335;
        static constexpr uint32 DroppedHordeFlag = 23334;
        static constexpr uint32 DroppedAllianceFlag = 23336;
    }

    static constexpr uint32 ObjectHordeFlag = 227740;
    static constexpr uint32 ObjectAllianceFlag = 227741;

    static constexpr Position FSB_WSG_GRAVEYARD_ALLIANCE(1410.592f, 1562.315f, 328.3285f, 0.0f);
    static constexpr Position FSB_WSG_GRAVEYARD_HORDE(1058.005f, 1373.705f, 328.5538f, 0.0f);

    static constexpr Position FSB_WSG_BASE_ALLIANCE(1516.0f, 1481.0f, 352.0f, 0.0f);
    static constexpr Position FSB_WSG_BASE_HORDE(930.0f, 1437.567f, 345.536f, 0.0f);

    static constexpr Position FSB_WSG_FLAG_ALLIANCE(1540.4499f, 1481.2199f, 345.536f, 0.0f);
    static constexpr Position FSB_WSG_FLAG_HORDE(917.0f, 1434.0f, 346.1829f, 0.0f);

    static constexpr Position FSB_WSG_CENTER(1212.0f, 1469.0f, 345.536f, 0.0f);

    void SpawnBots(Battleground* battleground, Player* triggeringPlayer = nullptr);

    void OnMovementInform(Creature* bot, FSB_BattlegroundData* bgData, uint32 type, uint32 id);

    void InitializeBot(Creature* bot, FSB_BattlegroundData* bgData, Battleground* bg);
    void UpdateBot(Creature* bot, FSB_BattlegroundData* bgData);
    void SetBotState(Creature* bot, FSB_BattlegroundData* bgData, WSGState newState);

    // Called by FSB_EVENT_WSG_USE_FLAG (1s after reaching the enemy flag position).
    void TryUseEnemyFlag(Creature* bot, FSB_BattlegroundData* bgData);
}
