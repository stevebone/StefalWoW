/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRINITYCORE_MOVEMENT_NODES_H
#define TRINITYCORE_MOVEMENT_NODES_H

#include "AI/BehaviorTree/BehaviorTree.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Unit.h"
#include "MotionMaster.h"
#include "PathGenerator.h"
#include "Duration.h"
#include "ObjectAccessor.h"

namespace Playerbot
{

/**
 * @brief Move to target position
 */
class TC_GAME_API BTMoveToPosition : public BTLeaf
{
public:
    BTMoveToPosition(float acceptableDistance = 1.0f)
        : BTLeaf("MoveToPosition"), _acceptableDistance(acceptableDistance), _movementStarted(false)
    {}

    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        if (!ai)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Player* bot = ai->GetBot();
        if (!bot)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        // Get target position from blackboard
        Position targetPos;
        if (!blackboard.Get<Position>("TargetPosition", targetPos))
        {
            _status = BTStatus::FAILURE;
            Reset();
            return _status;
        }

        // Check if already at target
        float distance = bot->GetExactDist(&targetPos);
        if (distance <= _acceptableDistance)
        {
            _status = BTStatus::SUCCESS;
            Reset();
            return _status;
        }

        // Start movement if not already moving
        if (!_movementStarted)
        {
            // Use validated pathfinding for behavior tree movement
            if (!ai->MoveTo(targetPos, true))
            {
                // Fallback to legacy if validation fails
                bot->GetMotionMaster()->MovePoint(0, targetPos);
            }
            _movementStarted = true;
        }

        // Check if movement is complete
        if (!bot->isMoving())
        {
            // Reached destination or movement failed
            _status = (distance <= _acceptableDistance * 2.0f) ? BTStatus::SUCCESS : BTStatus::FAILURE;
            Reset();
        }
        else
        {
            _status = BTStatus::RUNNING;
        }

        return _status;
    }

    void Reset() override
    {
        BTLeaf::Reset();
        _movementStarted = false;
    }

private:
    float _acceptableDistance;
    bool _movementStarted;
};

/**
 * @brief Move to target unit
 */
class TC_GAME_API BTMoveToTarget : public BTLeaf
{
public:
    BTMoveToTarget(float minRange, float maxRange)
        : BTLeaf("MoveToTarget"), _minRange(minRange), _maxRange(maxRange), _movementStarted(false)
    {}

    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        if (!ai)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Player* bot = ai->GetBot();
        if (!bot)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        // Get target from blackboard
        Unit* target = nullptr;
        if (!blackboard.Get<Unit*>("CurrentTarget", target) || !target)
        {
            _status = BTStatus::FAILURE;
            Reset();
            return _status;
        }

        // Check current distance
        float distance = bot->GetDistance(target);

        // Already in range
        if (distance >= _minRange && distance <= _maxRange)
        {
            _status = BTStatus::SUCCESS;
            Reset();
            return _status;
        }

        // Start movement if not already moving
        if (!_movementStarted)
        {
            // Calculate optimal position
            float optimalDistance = (_minRange + _maxRange) / 2.0f;

            // Use validated pathfinding for optimal range following
            if (!ai->MoveToUnit(target, optimalDistance))
            {
                // Fallback to legacy if validation fails
                bot->GetMotionMaster()->MoveFollow(target, optimalDistance, 0.0f);
            }
            _movementStarted = true;
        }

        // Check if in range now
        distance = bot->GetDistance(target);
        if (distance >= _minRange && distance <= _maxRange)
        {
            _status = BTStatus::SUCCESS;
            Reset();
        }
        else
        {
            _status = BTStatus::RUNNING;
        }

        return _status;
    }

    void Reset() override
    {
        BTLeaf::Reset();
        _movementStarted = false;
    }

private:
    float _minRange;
    float _maxRange;
    bool _movementStarted;
};

/**
 * @brief Check if path to target is clear (no obstacles)
 */
class TC_GAME_API BTCheckPathClear : public BTLeaf
{
public:
    BTCheckPathClear()
        : BTLeaf("CheckPathClear")
    {}

    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        if (!ai)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Player* bot = ai->GetBot();
        if (!bot)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        // Get target position
        Unit* target = nullptr;
        Position targetPos;

        if (blackboard.Get<Unit*>("CurrentTarget", target) && target)
        {
            targetPos = *target;
        }
        else if (!blackboard.Get<Position>("TargetPosition", targetPos))
        {
            _status = BTStatus::FAILURE;
            return _status;
        }

        // Generate path
        PathGenerator path(bot);
        path.CalculatePath(targetPos.GetPositionX(), targetPos.GetPositionY(), targetPos.GetPositionZ());

        if (path.GetPathType() & PATHFIND_NORMAL)
        {
            _status = BTStatus::SUCCESS;
        }
        else
        {
            _status = BTStatus::FAILURE;
        }

        return _status;
    }
};

/**
 * @brief Follow group leader or master
 */
class TC_GAME_API BTFollowLeader : public BTLeaf
{
public:
    BTFollowLeader(float followDistance = 3.0f)
        : BTLeaf("FollowLeader"), _followDistance(followDistance), _movementStarted(false)
    {}

    BTStatus Tick(BotAI* ai, BTBlackboard& /*blackboard*/) override
    {
        if (!ai)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Player* bot = ai->GetBot();
        if (!bot)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        // Get leader (group leader)
        Player* leader = nullptr;
        Group* group = bot->GetGroup();
        if (group)
        {
            ObjectGuid leaderGuid = group->GetLeaderGUID();
            leader = ObjectAccessor::FindPlayer(leaderGuid);
        }

        if (!leader)
        {
            _status = BTStatus::FAILURE;
            Reset();
            return _status;
        }

        // Check if already following at correct distance
        float distance = bot->GetDistance(leader);
        if (distance <= _followDistance)
        {
            _status = BTStatus::SUCCESS;
            Reset();
            return _status;
        }

        // Start following if not already
        if (!_movementStarted)
        {
            // Use validated pathfinding for leader following
            if (!ai->MoveToUnit(leader, _followDistance))
            {
                // Fallback to legacy if validation fails
                bot->GetMotionMaster()->MoveFollow(leader, _followDistance, bot->GetFollowAngle());
            }
            _movementStarted = true;
        }

        // Check if in range now
        distance = bot->GetDistance(leader);
        if (distance <= _followDistance)
        {
            _status = BTStatus::SUCCESS;
            Reset();
        }
        else
        {
            _status = BTStatus::RUNNING;
        }

        return _status;
    }

    void Reset() override
    {
        BTLeaf::Reset();
        _movementStarted = false;
    }

private:
    float _followDistance;
    bool _movementStarted;
};

/**
 * @brief Stop all movement
 */
class TC_GAME_API BTStopMovement : public BTAction
{
public:
    BTStopMovement()
        : BTAction("StopMovement",
            [](BotAI* ai, BTBlackboard& /*blackboard*/) -> BTStatus
            {
                if (!ai)
                    return BTStatus::INVALID;

                Player* bot = ai->GetBot();
                if (!bot)
                    return BTStatus::INVALID;

                bot->StopMoving();
                bot->GetMotionMaster()->Clear();
                bot->GetMotionMaster()->MoveIdle();

                return BTStatus::SUCCESS;
            })
    {}
};

/**
 * @brief Check if bot is moving
 */
class TC_GAME_API BTCheckIsMoving : public BTCondition
{
public:
    BTCheckIsMoving()
        : BTCondition("CheckIsMoving",
            [](BotAI* ai, BTBlackboard& /*blackboard*/) -> bool
            {
                if (!ai)
                    return false;

                Player* bot = ai->GetBot();
                if (!bot)
                    return false;

                return bot->isMoving();
            })
    {}
};

/**
 * @brief Flee from target (kite backwards)
 */
class TC_GAME_API BTFleeFromTarget : public BTLeaf
{
public:
    BTFleeFromTarget(float fleeDistance)
        : BTLeaf("FleeFromTarget"), _fleeDistance(fleeDistance), _movementStarted(false)
    {}

    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        if (!ai)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Player* bot = ai->GetBot();
        if (!bot)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        // Get target to flee from
        Unit* target = nullptr;
        if (!blackboard.Get<Unit*>("CurrentTarget", target) || !target)
        {
            _status = BTStatus::FAILURE;
            Reset();
            return _status;
        }

        // Check if already far enough
        float distance = bot->GetDistance(target);
        if (distance >= _fleeDistance)
        {
            _status = BTStatus::SUCCESS;
            Reset();
            return _status;
        }

        // Start fleeing if not already
        if (!_movementStarted)
        {
            bot->GetMotionMaster()->MoveFleeing(target, Milliseconds(5000)); // Flee for 5 seconds
            _movementStarted = true;
        }

        // Check if far enough now
        distance = bot->GetDistance(target);
        if (distance >= _fleeDistance)
        {
            _status = BTStatus::SUCCESS;
            Reset();
        }
        else
        {
            _status = BTStatus::RUNNING;
        }

        return _status;
    }

    void Reset() override
    {
        BTLeaf::Reset();
        _movementStarted = false;
    }

private:
    float _fleeDistance;
    bool _movementStarted;
};

/**
 * @brief Move to healer position (stay in healing range)
 */
class TC_GAME_API BTMoveToHealerRange : public BTLeaf
{
public:
    BTMoveToHealerRange(float maxRange = 35.0f)
        : BTLeaf("MoveToHealerRange"), _maxRange(maxRange), _movementStarted(false)
    {}

    BTStatus Tick(BotAI* ai, BTBlackboard& /*blackboard*/) override
    {
        if (!ai)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Player* bot = ai->GetBot();
        if (!bot)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        // Find nearest healer in group
        Group* group = bot->GetGroup();
        if (!group)
        {
            _status = BTStatus::FAILURE;
            Reset();
            return _status;
        }

        Player* nearestHealer = nullptr;
        float nearestDistance = 999.0f;

        for (GroupReference& ref : group->GetMembers())
        {
            Player* member = ref.GetSource();
            if (!member || !member->IsInWorld() || member == bot)
                continue;

            // Check if member is healer
            uint8 classId = member->GetClass();
            uint8 spec = uint8(member->GetPrimarySpecialization());

            bool isHealer = false;
            if (classId == CLASS_PRIEST && (spec == 1 || spec == 2)) isHealer = true;
            if (classId == CLASS_PALADIN && spec == 0) isHealer = true;
            if (classId == CLASS_SHAMAN && spec == 2) isHealer = true;
            if (classId == CLASS_DRUID && spec == 2) isHealer = true;

            if (!isHealer)
                continue;

            float distance = bot->GetDistance(member);
            if (distance < nearestDistance)
            {
                nearestHealer = member;
                nearestDistance = distance;
            }
        }

        if (!nearestHealer)
        {
            _status = BTStatus::FAILURE;
            Reset();
            return _status;
        }

        // Check if already in range
        if (nearestDistance <= _maxRange)
        {
            _status = BTStatus::SUCCESS;
            Reset();
            return _status;
        }

        // Move towards healer
        if (!_movementStarted)
        {
            // Use validated pathfinding for moving towards healer
            if (!ai->MoveToUnit(nearestHealer, _maxRange * 0.8f))
            {
                // Fallback to legacy if validation fails
                bot->GetMotionMaster()->MoveFollow(nearestHealer, _maxRange * 0.8f, 0.0f);
            }
            _movementStarted = true;
        }

        // Check if in range now
        if (bot->GetDistance(nearestHealer) <= _maxRange)
        {
            _status = BTStatus::SUCCESS;
            Reset();
        }
        else
        {
            _status = BTStatus::RUNNING;
        }

        return _status;
    }

    void Reset() override
    {
        BTLeaf::Reset();
        _movementStarted = false;
    }

private:
    float _maxRange;
    bool _movementStarted;
};

/**
 * @brief Position behind target (for rogues/feral druids)
 */
class TC_GAME_API BTPositionBehindTarget : public BTLeaf
{
public:
    BTPositionBehindTarget(float distance = 2.0f)
        : BTLeaf("PositionBehindTarget"), _distance(distance), _movementStarted(false)
    {}

    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        if (!ai)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Player* bot = ai->GetBot();
        if (!bot)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Unit* target = nullptr;
        if (!blackboard.Get<Unit*>("CurrentTarget", target) || !target)
        {
            _status = BTStatus::FAILURE;
            Reset();
            return _status;
        }

        // Check if already behind target
        if (target->isInBack(bot))
        {
            _status = BTStatus::SUCCESS;
            Reset();
            return _status;
        }

        // Move to position behind target
        if (!_movementStarted)
        {
            float angle = target->GetOrientation() + static_cast<float>(M_PI); // 180 degrees behind
            float x = target->GetPositionX() + _distance * ::std::cos(angle);
            float y = target->GetPositionY() + _distance * ::std::sin(angle);
            float z = target->GetPositionZ();

            // Use validated pathfinding for circling movement
            Position dest(x, y, z, 0.0f);
            if (!ai->MoveTo(dest, true))
            {
                // Fallback to legacy if validation fails
                bot->GetMotionMaster()->MovePoint(0, x, y, z);
            }
            _movementStarted = true;
        }

        // Check if behind target now
        if (target->isInBack(bot))
        {
            _status = BTStatus::SUCCESS;
            Reset();
        }
        else
        {
            _status = BTStatus::RUNNING;
        }

        return _status;
    }

    void Reset() override
    {
        BTLeaf::Reset();
        _movementStarted = false;
    }

private:
    float _distance;
    bool _movementStarted;
};

/**
 * @brief Calculate safe position away from AoE/bad stuff
 */
class TC_GAME_API BTFindSafePosition : public BTLeaf
{
public:
    BTFindSafePosition(float searchRadius = 15.0f)
        : BTLeaf("FindSafePosition"), _searchRadius(searchRadius)
    {}

    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        if (!ai)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Player* bot = ai->GetBot();
        if (!bot)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        // ENHANCEMENT: Ground AoE detection system
        // Future implementation should:
        // - Query DynamicObject entries in vicinity (e.g., Death and Decay, Blizzard, Consecration)
        // - Check AreaTrigger objects for hazardous zones (e.g., Defile, Ring of Frost)
        // - Calculate safe positions outside detected AoE radius
        // - Account for multiple overlapping AoE effects
        // For now, find position away from current location
        float angle = frand(0.0f, 2.0f * static_cast<float>(M_PI));
        float distance = frand(_searchRadius * 0.5f, _searchRadius);

        float x = bot->GetPositionX() + distance * ::std::cos(angle);
        float y = bot->GetPositionY() + distance * ::std::sin(angle);
        float z = bot->GetPositionZ();

        bot->UpdateGroundPositionZ(x, y, z);

        Position safePos(x, y, z);
        blackboard.Set<Position>("TargetPosition", safePos);

        _status = BTStatus::SUCCESS;
        return _status;
    }

private:
    float _searchRadius;
};

/**
 * @brief Check if bot is too far from group
 */
class TC_GAME_API BTCheckTooFarFromGroup : public BTLeaf
{
public:
    BTCheckTooFarFromGroup(float maxDistance = 50.0f)
        : BTLeaf("CheckTooFarFromGroup"), _maxDistance(maxDistance)
    {}

    BTStatus Tick(BotAI* ai, BTBlackboard& /*blackboard*/) override
    {
        if (!ai)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Player* bot = ai->GetBot();
        if (!bot)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        Group* group = bot->GetGroup();
        if (!group)
        {
            _status = BTStatus::FAILURE;
            return _status;
        }

        // Check average distance to group members
        float totalDistance = 0.0f;
        uint32 memberCount = 0;

        for (GroupReference& ref : group->GetMembers())
        {
            Player* member = ref.GetSource();
            if (!member || !member->IsInWorld() || member == bot)
                continue;

            totalDistance += bot->GetDistance(member);
            memberCount++;
        }

        if (memberCount == 0)
        {
            _status = BTStatus::FAILURE;
            return _status;
        }

        float averageDistance = totalDistance / memberCount;

        _status = (averageDistance > _maxDistance) ? BTStatus::SUCCESS : BTStatus::FAILURE;
        return _status;
    }

private:
    float _maxDistance;
};

} // namespace Playerbot

#endif // TRINITYCORE_MOVEMENT_NODES_H
