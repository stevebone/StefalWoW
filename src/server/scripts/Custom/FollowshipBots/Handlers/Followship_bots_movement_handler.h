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

#include "Followship_bots_defines.h"

namespace FSBMovement
{
    // Movement point IDs
    static constexpr uint32 MOVEMENT_POINT_BOSS_DISTANCE = 0;
    static constexpr uint32 MOVEMENT_POINT_HIRED_LEAVE = 1;
    static constexpr uint32 MOVEMENT_POINT_DRUID_ROOTS_ESCAPE = 3;

    FSB_MovementStates GetBotMoveState(Creature* bot);

    void StopFollow(Creature* bot);
    void ResumeFollow(Creature* bot, float followDistance, float followAngle);

    bool EnsureInRange(Creature* bot, Unit* target);
    bool EnsureInRange(Creature* bot, Unit* target, float range);
    bool EnsureUnitDistance(Creature* bot, Unit* target, float minDistance);

    MovementGeneratorType GetMovementType(Unit* me);

    void BotSetMountedState(Creature* bot, bool& botMounted);

    void BotHandleReturnMovement(Creature* bot);
}
