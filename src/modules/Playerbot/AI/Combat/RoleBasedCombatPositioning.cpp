/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotThreatManager.h"
#include "GameTime.h"
#include "RoleBasedCombatPositioning.h"
#include "../ClassAI/PositionStrategyBase.h"
#include "Player.h"
#include "Unit.h"
#include "Group.h"
#include "LFG.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Map.h"
#include "MotionMaster.h"
#include "MovementGenerator.h"
#include "Log.h"
#include "World.h"
#include "DBCEnums.h"
#include "Movement/UnifiedMovementCoordinator.h"
#include "../../Movement/Arbiter/MovementPriorityMapper.h"
#include "../BotAI.h"
#include "Core/PlayerBotHelpers.h"
#include "UnitAI.h"
#include <algorithm>
#include <cmath>
#include <execution>

namespace Playerbot
{

// TankPositioning Implementation
TankPositioning::TankPositioning(const TankPositionConfig& config)
    : _config(config)
{
}

Position TankPositioning::CalculateTankPosition(Unit* target, Group* group, const CombatPositionContext& context)
{
    if (!target || !group)
        return {};

    // Calculate optimal facing angle to face boss away from group
    ::std::vector<Player*> groupMembers;
    for (GroupReference const& ref : group->GetMembers())
        if (Player* member = ref.GetSource())
            if (member->IsAlive() && !member->IsGameMaster())
                groupMembers.push_back(member);

    float optimalFacing = CalculateOptimalFacing(target, groupMembers);

    // Calculate position in front of boss at optimal distance
    Position tankPos = CalculateFrontalPosition(target, _config.optimalDistance);

    // Adjust for cleave mechanics
    if (_config.handleCleave && context.cleaveAngle > 0)
    {
        Position cleaveAvoidPos = CalculateCleaveAvoidancePosition(target, context.cleaveAngle);
        if (IsGroupSafeFromCleave(target, cleaveAvoidPos, context.cleaveAngle))
            tankPos = cleaveAvoidPos;
    }

    // Validate position
    if (!ValidateTankPosition(tankPos, target, context))
        tankPos = FindOptimalTankSpot(target, _config.optimalDistance, _config.maxDistance);

    return tankPos;
}

Position TankPositioning::CalculateOffTankPosition(Unit* target, Player* mainTank, const CombatPositionContext& context)
{
    if (!target || !mainTank)
        return {};

    // Off-tank positions opposite of main tank for swap mechanics
    Position mainTankPos = mainTank->GetPosition();
    float angleToMainTank = target->GetRelativeAngle(&mainTankPos);
    float offTankAngle = Position::NormalizeOrientation(angleToMainTank + static_cast<float>(M_PI));

    // Calculate position at swap distance
    Position offTankPos;
    offTankPos.m_positionX = target->GetPositionX() + _config.swapDistance * cos(offTankAngle);
    offTankPos.m_positionY = target->GetPositionY() + _config.swapDistance * sin(offTankAngle);
    offTankPos.m_positionZ = target->GetPositionZ();
    // Ensure position is valid
    if (!ValidateTankPosition(offTankPos, target, context))
    {
        // Try alternative positions
    for (float adjustment = 0.1f; adjustment <= static_cast<float>(M_PI) / 2.0f; adjustment += 0.1f)
        {
            for (int direction : {1, -1})
            {
                float testAngle = Position::NormalizeOrientation(offTankAngle + (adjustment * direction));
                offTankPos.m_positionX = target->GetPositionX() + _config.swapDistance * cos(testAngle);
                offTankPos.m_positionY = target->GetPositionY() + _config.swapDistance * sin(testAngle);
                if (ValidateTankPosition(offTankPos, target, context))
                    return offTankPos;
            }
        }
    }

    return offTankPos;
}

void TankPositioning::HandleThreatPositioning(Player* tank, Unit* target)
{
    if (!tank || !target)
        return;

    // Ensure tank maintains highest threat
    if (target->GetThreatManager().GetCurrentVictim() != tank)
    {
        // Tank needs to increase threat generation
        // This would trigger taunt or increased threat abilities
        TC_LOG_DEBUG("bot.playerbot", "Tank {} needs to establish threat on {}",
                     tank->GetName(), target->GetName());
    }

    // Check if boss needs to be rotated
    float currentFacing = target->GetOrientation();
    float desiredFacing = tank->GetRelativeAngle(target) + static_cast<float>(M_PI);
    if (ShouldRotateBoss(target, currentFacing, desiredFacing))
    {
        // Tank should reposition to rotate boss
        Position newPos = CalculateFrontalPosition(target, _config.optimalDistance);
        // PHASE 6B: Use Movement Arbiter with ROLE_POSITIONING priority (170)
        BotAI* botAI = dynamic_cast<BotAI*>(tank->GetAI());
        if (botAI && botAI->GetUnifiedMovementCoordinator())
        {
            bool accepted = botAI->RequestPointMovement(
                PlayerBotMovementPriority::ROLE_POSITIONING,  // Priority 170 - HIGH tier
                newPos,
                "Tank boss rotation positioning",
                "RoleBasedCombatPositioning");

            if (!accepted)
            {
                TC_LOG_TRACE("playerbot.movement.arbiter",
                    "RoleBasedCombatPositioning: Tank rotation rejected for {} - higher priority active",
                    tank->GetName());
            }
        }
        else
        {
            // FALLBACK: Use BotMovementController with validated pathfinding
            if (BotAI* ai = GetBotAI(tank))
            {
                if (!ai->MoveTo(newPos, true))
                {
                    // Final fallback to legacy if validation fails
                    tank->GetMotionMaster()->MovePoint(0, newPos);
                }
            }
            else
            {
                // Non-bot player - use standard movement
                tank->GetMotionMaster()->MovePoint(0, newPos);
            }
        }
    }
}

float TankPositioning::CalculateOptimalFacing(Unit* target, const ::std::vector<Player*>& groupMembers)
{
    if (groupMembers.empty())
        return target->GetOrientation();

    // Calculate center of group
    float sumX = 0, sumY = 0;
    for (const Player* member : groupMembers)
    {
        sumX += member->GetPositionX();
        sumY += member->GetPositionY();
    }

    Position groupCenter;
    groupCenter.m_positionX = sumX / groupMembers.size();
    groupCenter.m_positionY = sumY / groupMembers.size();
    groupCenter.m_positionZ = target->GetPositionZ();
    // Boss should face opposite of group center
    float angleToGroup = target->GetRelativeAngle(&groupCenter);
    return Position::NormalizeOrientation(angleToGroup + static_cast<float>(M_PI));
}
bool TankPositioning::ShouldRotateBoss(Unit* target, float currentFacing, float desiredFacing)
{
    float angleDiff = ::std::abs(Position::NormalizeOrientation(currentFacing - desiredFacing));

    // Only rotate if angle difference is significant (> 30 degrees)
    return angleDiff > (static_cast<float>(M_PI) / 6.0f);
}

void TankPositioning::ManageCleaveMechanics(Unit* target, const ::std::vector<Player*>& groupMembers)
{
    if (!_config.handleCleave || !target)
        return;

    float cleaveAngle = _config.cleaveAngle * (static_cast<float>(M_PI) / 180.0f);

    for (Player* member : groupMembers)
    {
        // Check if player is in cleave danger (inline implementation)
        Position memberPos = member->GetPosition();
        float angleToPos = target->GetRelativeAngle(&memberPos);
        float targetFacing = target->GetOrientation();
        float angleDiff = ::std::abs(Position::NormalizeOrientation(angleToPos - targetFacing));

        if (angleDiff < cleaveAngle / 2)
        {
            TC_LOG_DEBUG("bot.playerbot", "Player {} in cleave danger from {}",
                         member->GetName(), target->GetName());
            // Player should move out of cleave range
        }
    }
}

Position TankPositioning::CalculateCleaveAvoidancePosition(Unit* target, float cleaveAngle)
{
    // Calculate position that puts cleave cone away from predicted group location
    float safeAngle = Position::NormalizeOrientation(target->GetOrientation() + static_cast<float>(M_PI) + (cleaveAngle / 2) + CLEAVE_SAFETY_MARGIN);
    Position safePos;
    safePos.m_positionX = target->GetPositionX() + _config.optimalDistance * cos(safeAngle);
    safePos.m_positionY = target->GetPositionY() + _config.optimalDistance * sin(safeAngle);
    safePos.m_positionZ = target->GetPositionZ();
    return safePos;
}

bool TankPositioning::IsGroupSafeFromCleave(Unit* target, const Position& tankPos, float cleaveAngle)
{
    if (!target)
        return false;

    // Calculate where cleave would hit from tank position
    float bossface = target->GetRelativeAngle(&tankPos);
    float cleaveStart = Position::NormalizeOrientation(bossface - cleaveAngle / 2);
    float cleaveEnd = Position::NormalizeOrientation(bossface + cleaveAngle / 2);

    // This is simplified - would need actual group member positions
    return true;
}

Position TankPositioning::CalculateTankSwapPosition(Player* currentTank, Player* swapTank, Unit* target)
{
    if (!currentTank || !swapTank || !target)
        return {};

    // Calculate position for smooth tank swap
    Position currentPos = currentTank->GetPosition();
    Position swapPos;

    // Swap tank should position at 90 degrees from current tank
    float angleOffset = static_cast<float>(M_PI) / 2.0f;
    float swapAngle = target->GetRelativeAngle(&currentPos) + angleOffset;

    swapPos.m_positionX = target->GetPositionX() + _config.swapDistance * cos(swapAngle);
    swapPos.m_positionY = target->GetPositionY() + _config.swapDistance * sin(swapAngle);
    swapPos.m_positionZ = target->GetPositionZ();
    return swapPos;
}
bool TankPositioning::IsInSwapPosition(Player* tank, Player* otherTank, Unit* target)
{
    if (!tank || !otherTank || !target)
        return false;

    float distance = ::std::sqrt(tank->GetExactDistSq(otherTank)); // Calculate once from squared distance
    float angleiBetween = ::std::abs(target->GetRelativeAngle(tank) - target->GetRelativeAngle(otherTank));

    // Tanks should be at correct distance and angle for swap
    return distance >= _config.swapDistance * 0.8f &&
           distance <= _config.swapDistance * 1.2f &&
           angleiBetween >= (static_cast<float>(M_PI) / 3.0f);  // At least 60 degrees apart
}

Position TankPositioning::FindOptimalTankSpot(Unit* target, float minDistance, float maxDistance)
{
    if (!target)
        return {};

    Position bestPos;
    float bestScore = -1.0f;

    // Generate candidate positions
    for (int angle = 0; angle < 360; angle += 15)
    {
        float radians = angle * (static_cast<float>(M_PI) / 180.0f);
        float distance = (minDistance + maxDistance) / 2;

        Position candidatePos;
        candidatePos.m_positionX = target->GetPositionX() + distance * cos(radians);
        candidatePos.m_positionY = target->GetPositionY() + distance * sin(radians);
        candidatePos.m_positionZ = target->GetPositionZ();
        // Simple scoring - would be more complex in real implementation
        float score = 100.0f;

        // Prefer positions that face boss away from raid
    if (angle > 90 && angle < 270)
            score += 50.0f;

        if (score > bestScore)
        {
            bestScore = score;
            bestPos = candidatePos;
        }
    }

    return bestPos;
}

::std::vector<RolePositionScore> TankPositioning::EvaluateTankPositions(
    const ::std::vector<Position>& candidates, const CombatPositionContext& context)
{
    ::std::vector<RolePositionScore> scores;
    scores.reserve(candidates.size());

    for (const Position& pos : candidates)
    {
        RolePositionScore score;
        score.position = pos;
        score.isValid = ValidateTankPosition(pos, context.primaryTarget, context);

        if (score.isValid)
        {
            score.roleScore = ScoreTankPosition(pos, context);
            score.mechanicScore = 100.0f;  // Base mechanic score
            score.safetyScore = 100.0f;    // Tanks prioritize threat over safety
            score.efficiencyScore = 100.0f;  // Tank efficiency
            score.mobilityScore = 50.0f;    // Tanks have lower mobility needs

            // Check requirements
    if (EnumFlag<PositionalRequirement>(context.requirements).HasFlag(PositionalRequirement::FRONT_OF_TARGET))
            {
                float angle = context.primaryTarget->GetRelativeAngle(&pos);
                if (angle < static_cast<float>(M_PI) / 4.0f || angle > 7.0f * static_cast<float>(M_PI) / 4.0f)
                    score.metRequirements = static_cast<PositionalRequirement>(static_cast<uint32>(score.metRequirements) | static_cast<uint32>(PositionalRequirement::FRONT_OF_TARGET));
                else
                    score.failedRequirements = static_cast<PositionalRequirement>(static_cast<uint32>(score.failedRequirements) | static_cast<uint32>(PositionalRequirement::FRONT_OF_TARGET));
            }

            score.totalScore = score.roleScore * 0.4f + score.mechanicScore * 0.3f +
                              score.safetyScore * 0.1f + score.efficiencyScore * 0.1f +
                              score.mobilityScore * 0.1f;
        }

        scores.push_back(score);
    }

    return scores;
}

Position TankPositioning::CalculateFrontalPosition(Unit* target, float distance)
{
    if (!target)
        return {};

    Position pos;
    pos.m_positionX = target->GetPositionX() + distance * cos(target->GetOrientation());
    pos.m_positionY = target->GetPositionY() + distance * sin(target->GetOrientation());
    pos.m_positionZ = target->GetPositionZ();
    return pos;
}

float TankPositioning::CalculateThreatAngle(const Position& tankPos, const Position& targetPos,
                                            const ::std::vector<Player*>& group)
{
    if (group.empty())
        return 0.0f;
    // Calculate average angle to group members
    float sumAngle = 0;
    for (const Player* member : group)
    {
        float angle = ::std::atan2(member->GetPositionY() - targetPos.m_positionY,
                                 member->GetPositionX() - targetPos.m_positionX);
        sumAngle += angle;
    }

    float avgGroupAngle = sumAngle / group.size();
    float tankAngle = ::std::atan2(tankPos.m_positionY - targetPos.m_positionY,
                                 tankPos.m_positionX - targetPos.m_positionX);

    return ::std::abs(Position::NormalizeOrientation(tankAngle - avgGroupAngle - static_cast<float>(M_PI)));
}

bool TankPositioning::ValidateTankPosition(const Position& pos, Unit* target,
                                          const CombatPositionContext& context)
{
    if (!target)
        return false;

    // Check distance (use squared distance for comparison)
    float distSq = target->GetExactDistSq(pos);
    if (distSq < (MIN_TANK_DISTANCE * MIN_TANK_DISTANCE) || distSq > (MAX_TANK_DISTANCE * MAX_TANK_DISTANCE))
        return false;

    // Check if position is in front of target
    float angle = target->GetRelativeAngle(&pos);
    if (angle > static_cast<float>(M_PI) / 3.0f && angle < 2.0f * static_cast<float>(M_PI) - static_cast<float>(M_PI) / 3.0f)
        return false;

    // Additional validation would include walkable checks, LOS, etc.
    return true;
}

float TankPositioning::ScoreTankPosition(const Position& pos, const CombatPositionContext& context)
{
    if (!context.primaryTarget)
        return 0.0f;

    float score = 100.0f;

    // Distance score
    float distance = ::std::sqrt(context.primaryTarget->GetExactDistSq(pos)); // Calculate once from squared distance
    float distanceScore = 100.0f - ::std::abs(distance - _config.optimalDistance) * 10.0f;
    score += distanceScore * 0.3f;

    // Threat angle score (facing boss away from group)
    float threatAngle = CalculateThreatAngle(pos, context.primaryTarget->GetPosition(), context.tanks);
    float angleScore = (1.0f - (threatAngle / static_cast<float>(M_PI))) * 100.0f;
    score += angleScore * 0.4f;

    // Stability score (minimize movement)
    if (context.bot)
    {
        float moveDistance = ::std::sqrt(context.bot->GetExactDistSq(pos)); // Calculate once from squared distance
        float stabilityScore = 100.0f - moveDistance * 2.0f;
        score += stabilityScore * 0.3f;
    }

    return ::std::max(0.0f, ::std::min(100.0f, score));
}

// HealerPositioning Implementation
HealerPositioning::HealerPositioning(const HealerPositionConfig& config)
    : _config(config)
{
}

Position HealerPositioning::CalculateHealerPosition(Group* group, Unit* combatTarget,
                                                   const CombatPositionContext& context)
{
    if (!group)
        return {};

    ::std::vector<Player*> allies;
    for (GroupReference const& ref : group->GetMembers())
        if (Player* member = ref.GetSource())
            if (member->IsAlive())
                allies.push_back(member);

    // Find optimal position for healing coverage
    Position healerPos = OptimizeHealingCoverage(context.bot, allies);

    // Ensure safety from threat
    if (combatTarget)
    {
        float distanceToThreat = healerPos.GetExactDist(combatTarget->GetPosition());
        if (distanceToThreat < _config.minSafeDistance)
        {
            healerPos = FindSafeHealingSpot(context.bot, combatTarget, context);
        }
    }

    // Validate line of sight
    if (!HasLineOfSightToAll(healerPos, allies))
    {
        healerPos = FindBestLOSPosition(context.bot, allies);
    }

    return healerPos;
}

Position HealerPositioning::CalculateRaidHealerPosition(const ::std::vector<Player*>& raidMembers,
                                                       const CombatPositionContext& context)
{
    if (raidMembers.empty())
        return {};

    // Calculate center of care for raid healing
    Position centerOfCare = CalculateCenterOfCare(raidMembers);

    // Adjust for safety
    Position safePos = centerOfCare;
    if (context.primaryTarget)
    {
        float distanceToThreat = centerOfCare.GetExactDist(context.primaryTarget->GetPosition());
        if (distanceToThreat < _config.minSafeDistance)
        {
            // Move away from threat
            float angle = context.primaryTarget->GetRelativeAngle(&centerOfCare);
            safePos.m_positionX = context.primaryTarget->GetPositionX() +
                                  _config.optimalRange * cos(angle);
            safePos.m_positionY = context.primaryTarget->GetPositionY() +
                                  _config.optimalRange * sin(angle);
        }
    }

    return safePos;
}

Position HealerPositioning::CalculateTankHealerPosition(Player* tank, Unit* threat,
                                                       const CombatPositionContext& context)
{
    if (!tank)
        return {};

    Position healerPos;

    // Position behind and to the side of tank for optimal LOS
    float tankFacing = tank->GetOrientation();
    float healerAngle = Position::NormalizeOrientation(tankFacing + 3.0f * static_cast<float>(M_PI) / 4.0f);

    healerPos.m_positionX = tank->GetPositionX() + _config.optimalRange * cos(healerAngle);
    healerPos.m_positionY = tank->GetPositionY() + _config.optimalRange * sin(healerAngle);
    healerPos.m_positionZ = tank->GetPositionZ();
    // Ensure safety from threat
    if (threat && healerPos.GetExactDist(threat->GetPosition()) < _config.minSafeDistance)
    {
        healerPos = FindSafeHealingSpot(context.bot, threat, context);
    }

    return healerPos;
}

bool HealerPositioning::IsInOptimalHealingRange(Player* healer, const ::std::vector<Player*>& allies)
{
    if (!healer || allies.empty())
        return false;

    float maxDistSq = 0.0f;
    for (const Player* ally : allies)
    {
        float distSq = healer->GetExactDistSq(ally);
        maxDistSq = ::std::max(maxDistSq, distSq);
    }

    return maxDistSq <= (_config.optimalRange * _config.optimalRange);
}

float HealerPositioning::CalculateHealingCoverage(const Position& healerPos,
                                                 const ::std::vector<Player*>& allies)
{
    if (allies.empty())
        return 0.0f;

    int inRange = 0;
    for (const Player* ally : allies)
    {
        if (healerPos.GetExactDist(ally->GetPosition()) <= _config.optimalRange)
            inRange++;
    }

    return (float)inRange / allies.size() * 100.0f;
}

Position HealerPositioning::OptimizeHealingCoverage(Player* healer, const ::std::vector<Player*>& allies)
{
    if (!healer || allies.empty())
        return {};

    // Start with center of care
    Position optimalPos = CalculateCenterOfCare(allies);

    // Fine-tune position for maximum coverage
    float bestCoverage = CalculateHealingCoverage(optimalPos, allies);

    // Search nearby positions for better coverage
    for (int angle = 0; angle < 360; angle += 30)
    {
        for (float distance = 5.0f; distance <= 15.0f; distance += 5.0f)
        {
            Position testPos;
            float radians = angle * (static_cast<float>(M_PI) / 180.0f);
            testPos.m_positionX = optimalPos.m_positionX + distance * cos(radians);
            testPos.m_positionY = optimalPos.m_positionY + distance * sin(radians);
            testPos.m_positionZ = optimalPos.m_positionZ;

            float coverage = CalculateHealingCoverage(testPos, allies);
            if (coverage > bestCoverage)
            {
                bestCoverage = coverage;
                optimalPos = testPos;
            }
        }
    }

    return optimalPos;
}

Position HealerPositioning::FindSafeHealingSpot(Player* healer, Unit* threat,
                                               const CombatPositionContext& context)
{
    if (!healer || !threat)
        return {};

    // Calculate safe position away from threat
    float angleFromThreat = threat->GetRelativeAngle(healer);
    float safeDistance = ::std::max(_config.minSafeDistance, _config.optimalRange);

    Position safePos;
    safePos.m_positionX = threat->GetPositionX() + safeDistance * cos(angleFromThreat);
    safePos.m_positionY = threat->GetPositionY() + safeDistance * sin(angleFromThreat);
    safePos.m_positionZ = threat->GetPositionZ();

    // Validate position
    if (!IsPositionSafeForHealing(safePos, context))
    {
        // Search for alternative safe positions
    for (int angleOffset = 15; angleOffset <= 180; angleOffset += 15)
        {
            for (int direction : {1, -1})
            {
                float testAngle = Position::NormalizeOrientation(angleFromThreat + (angleOffset * direction * static_cast<float>(M_PI) / 180.0f));
                safePos.m_positionX = threat->GetPositionX() + safeDistance * cos(testAngle);
                safePos.m_positionY = threat->GetPositionY() + safeDistance * sin(testAngle);

                if (IsPositionSafeForHealing(safePos, context))
                    return safePos;
            }
        }
    }

    return safePos;
}

bool HealerPositioning::IsPositionSafeForHealing(const Position& pos, const CombatPositionContext& context)
{
    // Check distance from threats
    if (context.primaryTarget)
    {
        float distance = pos.GetExactDist(context.primaryTarget->GetPosition());
        if (distance < _config.minSafeDistance)
            return false;
    }

    // Check for danger zones
    for (const Position& danger : context.dangerZones)
    {
        if (pos.GetExactDist(danger) < 10.0f)  // 10 yard danger radius
            return false;
    }

    // Additional checks would include AOE zones, environmental hazards, etc.
    return true;
}

float HealerPositioning::CalculateSafetyScore(const Position& pos, const ::std::vector<Unit*>& threats)
{
    float safetyScore = 100.0f;

    for (const Unit* threat : threats)
    {
        float distance = pos.GetExactDist(threat->GetPosition());
        if (distance < _config.minSafeDistance)
        {
            float penalty = (1.0f - distance / _config.minSafeDistance) * 50.0f;
            safetyScore -= penalty;
        }
    }

    return ::std::max(0.0f, safetyScore);
}

void HealerPositioning::MaintainLineOfSight(Player* healer, const ::std::vector<Player*>& allies)
{
    if (!healer)
        return;

    for (Player* ally : allies)
    {
        if (!healer->IsWithinLOSInMap(ally))
        {
            TC_LOG_DEBUG("bot.playerbot", "Healer {} lost LOS to {}",
                         healer->GetName(), ally->GetName());
            // Trigger repositioning
        }
    }
}

bool HealerPositioning::HasLineOfSightToAll(const Position& healerPos, const ::std::vector<Player*>& allies)
{
    // Simplified - would need actual LOS checks
    for (const Player* ally : allies)
    {
        // Check if position would have LOS to ally
        float distance = healerPos.GetExactDist(ally->GetPosition());
        if (distance > _config.maxRange)
            return false;
    }

    return true;
}

Position HealerPositioning::FindBestLOSPosition(Player* healer, const ::std::vector<Player*>& priorityTargets)
{
    if (!healer || priorityTargets.empty())
        return {};

    Position bestPos = healer->GetPosition();
    int maxLOSTargets = 0;

    // Search for position with best LOS
    for (int angle = 0; angle < 360; angle += 30)
    {
        float radians = angle * (static_cast<float>(M_PI) / 180.0f);
        Position testPos;
        testPos.m_positionX = healer->GetPositionX() + 10.0f * cos(radians);
        testPos.m_positionY = healer->GetPositionY() + 10.0f * sin(radians);
        testPos.m_positionZ = healer->GetPositionZ();
        int losCount = 0;
        for (const Player* target : priorityTargets)
        {
            if (testPos.GetExactDist(target->GetPosition()) <= _config.optimalRange)
                losCount++;
        }

        if (losCount > maxLOSTargets)
        {
            maxLOSTargets = losCount;
            bestPos = testPos;
        }
    }

    return bestPos;
}

::std::vector<Position> HealerPositioning::CalculateMultiHealerPositions(
    const ::std::vector<Player*>& healers, const ::std::vector<Player*>& group)
{
    ::std::vector<Position> positions;
    if (healers.empty() || group.empty())
        return positions;
    // Divide healing assignments
    size_t healersPerSection = ::std::max(static_cast<size_t>(1), healers.size());
    size_t playersPerHealer = group.size() / healersPerSection;

    // Assign positions based on coverage zones
    for (size_t i = 0; i < healers.size(); ++i)
    {
        ::std::vector<Player*> assignment;
        size_t startIdx = i * playersPerHealer;
        size_t endIdx = ::std::min(startIdx + playersPerHealer, group.size());
        for (size_t j = startIdx; j < endIdx; ++j)
            assignment.push_back(group[j]);

        Position healerPos = OptimizeHealingCoverage(healers[i], assignment);
        positions.push_back(healerPos);
    }

    return positions;
}

void HealerPositioning::CoordinateHealerPositioning(const ::std::vector<Player*>& healers, Group* group)
{
    if (healers.empty() || !group)
        return;

    ::std::vector<Player*> groupMembers;
    for (GroupReference const& ref : group->GetMembers())
        if (Player* member = ref.GetSource())
            if (member->IsAlive())
                groupMembers.push_back(member);

    ::std::vector<Position> healerPositions = CalculateMultiHealerPositions(healers, groupMembers);

    for (size_t i = 0; i < healers.size() && i < healerPositions.size(); ++i)
    {
        // PHASE 6B: Use Movement Arbiter with ROLE_POSITIONING priority (170)
        BotAI* botAI = dynamic_cast<BotAI*>(healers[i]->GetAI());
        if (botAI && botAI->GetUnifiedMovementCoordinator())
        {
            bool accepted = botAI->RequestPointMovement(
                PlayerBotMovementPriority::ROLE_POSITIONING,  // Priority 170 - HIGH tier
                healerPositions[i],
                "Healer optimal healing coverage positioning",
                "RoleBasedCombatPositioning");

            if (!accepted)
            {
                TC_LOG_TRACE("playerbot.movement.arbiter",
                    "RoleBasedCombatPositioning: Healer positioning rejected for {} - higher priority active",
                    healers[i]->GetName());
            }
        }
        else
        {
            // FALLBACK: Use BotMovementController with validated pathfinding
            if (BotAI* ai = GetBotAI(healers[i]))
            {
                if (!ai->MoveTo(healerPositions[i], true))
                {
                    // Final fallback to legacy if validation fails
                    healers[i]->GetMotionMaster()->MovePoint(0, healerPositions[i]);
                }
            }
            else
            {
                // Non-bot player - use standard movement
                healers[i]->GetMotionMaster()->MovePoint(0, healerPositions[i]);
            }
        }
    }
}

::std::vector<RolePositionScore> HealerPositioning::EvaluateHealerPositions(
    const ::std::vector<Position>& candidates, const CombatPositionContext& context)
{
    ::std::vector<RolePositionScore> scores;
    scores.reserve(candidates.size());

    for (const Position& pos : candidates)
    {
        RolePositionScore score;
        score.position = pos;
        score.isValid = IsPositionSafeForHealing(pos, context);

        if (score.isValid)
        {
            score.roleScore = CalculateHealerScore(pos, context);
            score.mechanicScore = 100.0f;  // Base mechanic score
            score.safetyScore = CalculateSafetyScore(pos, {context.primaryTarget});
            score.efficiencyScore = CalculateHealingCoverage(pos, context.healers);
            score.mobilityScore = 80.0f;  // Healers need good mobility

            score.totalScore = score.roleScore * 0.3f + score.mechanicScore * 0.2f +
                              score.safetyScore * 0.3f + score.efficiencyScore * 0.15f +
                              score.mobilityScore * 0.05f;
        }

        scores.push_back(score);
    }

    return scores;
}

float HealerPositioning::CalculateHealerScore(const Position& pos, const CombatPositionContext& context)
{
    float score = 100.0f;

    // Coverage score
    ::std::vector<Player*> allPlayers = context.tanks;
    allPlayers.insert(allPlayers.end(), context.meleeDPS.begin(), context.meleeDPS.end());
    allPlayers.insert(allPlayers.end(), context.rangedDPS.begin(), context.rangedDPS.end());

    float coverage = CalculateHealingCoverage(pos, allPlayers);
    score += coverage * 0.5f;

    // Safety score
    if (context.primaryTarget)
    {
        float distance = pos.GetExactDist(context.primaryTarget->GetPosition());
        float safetyBonus = ::std::min(50.0f, (distance / _config.minSafeDistance) * 25.0f);
        score += safetyBonus;
    }

    return ::std::min(100.0f, score);
}

bool HealerPositioning::ValidateHealerPosition(const Position& pos, const CombatPositionContext& context)
{
    // Check basic safety
    if (!IsPositionSafeForHealing(pos, context))
        return false;

    // Check range to tanks (priority targets)
    for (const Player* tank : context.tanks)
    {
        if (pos.GetExactDist(tank->GetPosition()) > _config.maxRange)
            return false;
    }

    return true;
}

Position HealerPositioning::CalculateCenterOfCare(const ::std::vector<Player*>& allies)
{
    Position center;
    if (allies.empty())
        return center;

    float sumX = 0, sumY = 0, sumZ = 0;
    for (const Player* ally : allies)
    {
        sumX += ally->GetPositionX();
        sumY += ally->GetPositionY();
        sumZ += ally->GetPositionZ();
    }

    center.m_positionX = sumX / allies.size();
    center.m_positionY = sumY / allies.size();
    center.m_positionZ = sumZ / allies.size();

    return center;
}

float HealerPositioning::GetEffectiveHealingRange(Player* healer)
{
    if (!healer)
        return _config.optimalRange;

    // Would check healer's actual spell ranges
    return _config.optimalRange;
}

// DPSPositioning Implementation
DPSPositioning::DPSPositioning(const DPSPositionConfig& config)
    : _config(config)
{
}

Position DPSPositioning::CalculateMeleeDPSPosition(Unit* target, Player* tank,
                                                  const CombatPositionContext& context)
{
    if (!target)
        return {};

    Position dpsPos;

    if (_config.preferBehind)
    {
        dpsPos = CalculateBackstabPosition(target, _config.backstabAngle);
    }
    else if (_config.allowFlanking)
    {
        dpsPos = CalculateFlankPosition(target, context.meleeDPS.size() % 2 == 0);
    }
    else
    {
        // Default melee position
        float angle = target->GetOrientation() + static_cast<float>(M_PI);
        dpsPos = RotateAroundTarget(target, angle, _config.meleeOptimalDistance);
    }

    // Avoid cleave if tank is present
    if (tank && context.cleaveAngle > 0)
    {
        AvoidFrontalCleaves(context.bot, target, context.cleaveAngle);
    }

    return dpsPos;
}

Position DPSPositioning::CalculateBackstabPosition(Unit* target, float requiredAngle)
{
    if (!target)
        return {};

    // Calculate position behind target
    float targetFacing = target->GetOrientation();
    float backstabAngle = Position::NormalizeOrientation(targetFacing + static_cast<float>(M_PI));
    return RotateAroundTarget(target, backstabAngle, _config.meleeOptimalDistance);
}

Position DPSPositioning::CalculateFlankPosition(Unit* target, bool leftFlank)
{
    if (!target)
        return {};

    float targetFacing = target->GetOrientation();
    float flankAngle = Position::NormalizeOrientation(
        targetFacing + (leftFlank ? -static_cast<float>(M_PI)/2.0f : static_cast<float>(M_PI)/2.0f));

    return RotateAroundTarget(target, flankAngle, _config.meleeOptimalDistance);
}

void DPSPositioning::DistributeMeleePositions(const ::std::vector<Player*>& meleeDPS,
                                             Unit* target, Player* tank)
{
    if (!target || meleeDPS.empty())
        return;

    // Calculate available angles (avoiding front if tank is present)
    float startAngle = tank ? static_cast<float>(M_PI)/3.0f : 0;  // Start 60 degrees from front if tanked
    float endAngle = tank ? 5.0f*static_cast<float>(M_PI)/3.0f : 2.0f*static_cast<float>(M_PI);  // End 60 degrees from front
    float angleRange = endAngle - startAngle;
    float angleStep = angleRange / meleeDPS.size();

    for (size_t i = 0; i < meleeDPS.size(); ++i)
    {
        float angle = Position::NormalizeOrientation(
            target->GetOrientation() + startAngle + (i * angleStep));
        Position pos = RotateAroundTarget(target, angle, _config.meleeOptimalDistance);
        // PHASE 6B: Use Movement Arbiter with ROLE_POSITIONING priority (170)
        BotAI* botAI = dynamic_cast<BotAI*>(meleeDPS[i]->GetAI());
        if (botAI && botAI->GetUnifiedMovementCoordinator())
        {
            bool accepted = botAI->RequestPointMovement(
                PlayerBotMovementPriority::ROLE_POSITIONING,
                pos,
                "Melee DPS optimal positioning",
                "RoleBasedCombatPositioning");

            if (!accepted)
            {
                TC_LOG_TRACE("playerbot.movement.arbiter",
                    "RoleBasedCombatPositioning: Melee DPS positioning rejected for {} - higher priority active",
                    meleeDPS[i]->GetName());
            }
        }
        else
        {
            // FALLBACK: Use BotMovementController with validated pathfinding
            if (BotAI* ai = GetBotAI(meleeDPS[i]))
            {
                if (!ai->MoveTo(pos, true))
                {
                    // Final fallback to legacy if validation fails
                    meleeDPS[i]->GetMotionMaster()->MovePoint(0, pos);
                }
            }
            else
            {
                // Non-bot player - use standard movement
                meleeDPS[i]->GetMotionMaster()->MovePoint(0, pos);
            }
        }
    }
}
Position DPSPositioning::CalculateRangedDPSPosition(Unit* target, float optimalRange,
                                                   const CombatPositionContext& context)
{
    if (!target)
        return {};

    Position rangedPos;

    // Start with base position at optimal range
    float baseAngle = target->GetOrientation() + static_cast<float>(M_PI);
    rangedPos = RotateAroundTarget(target, baseAngle, optimalRange);
    // Apply spread if required
    if (_config.maintainSpread && context.requiresSpread)
    {
        // Adjust position to maintain spread from other ranged
    for (const Player* other : context.rangedDPS)
        {
            if (other != context.bot)
            {
                float distance = rangedPos.GetExactDist(other->GetPosition());
                if (distance < _config.spreadDistance)
                {
                    // Move away from other ranged DPS
                    float angleAway = other->GetRelativeAngle(&rangedPos);
                    rangedPos.m_positionX += (_config.spreadDistance - distance) * cos(angleAway);
                    rangedPos.m_positionY += (_config.spreadDistance - distance) * sin(angleAway);
                }
            }
        }
    }

    return rangedPos;
}

void DPSPositioning::SpreadRangedPositions(const ::std::vector<Player*>& rangedDPS,
                                          Unit* target, float spreadDistance)
{
    if (!target || rangedDPS.empty())
        return;

    // Distribute ranged DPS in an arc behind the boss
    float arcStart = 2.0f*static_cast<float>(M_PI)/3.0f;   // 120 degrees
    float arcEnd = 4.0f*static_cast<float>(M_PI)/3.0f;     // 240 degrees
    float arcRange = arcEnd - arcStart;
    float angleStep = rangedDPS.size() > 1 ? arcRange / (rangedDPS.size() - 1) : 0;

    for (size_t i = 0; i < rangedDPS.size(); ++i)
    {
        float angle = Position::NormalizeOrientation(
            target->GetOrientation() + arcStart + (i * angleStep));
        Position pos = RotateAroundTarget(target, angle, _config.rangedOptimalDistance);

        // PHASE 6B: Use Movement Arbiter with ROLE_POSITIONING priority (170)
        BotAI* botAI = dynamic_cast<BotAI*>(rangedDPS[i]->GetAI());
        if (botAI && botAI->GetUnifiedMovementCoordinator())
        {
            bool accepted = botAI->RequestPointMovement(
                PlayerBotMovementPriority::ROLE_POSITIONING,
                pos,
                "Ranged DPS optimal positioning",
                "RoleBasedCombatPositioning");

            if (!accepted)
            {
                TC_LOG_TRACE("playerbot.movement.arbiter",
                    "RoleBasedCombatPositioning: Ranged DPS optimal positioning rejected for {} - higher priority active",
                    rangedDPS[i]->GetName());
            }
        }
        else
        {
            // FALLBACK: Use BotMovementController with validated pathfinding
            if (BotAI* ai = GetBotAI(rangedDPS[i]))
            {
                if (!ai->MoveTo(pos, true))
                {
                    // Final fallback to legacy if validation fails
                    rangedDPS[i]->GetMotionMaster()->MovePoint(0, pos);
                }
            }
            else
            {
                // Non-bot player - use standard movement
                rangedDPS[i]->GetMotionMaster()->MovePoint(0, pos);
            }
        }
    }
}

Position DPSPositioning::CalculateCasterPosition(Player* caster, Unit* target, uint32 spellId)
{
    if (!caster || !target)
        return {};
    // Get spell info for range requirements
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return CalculateRangedDPSPosition(target, _config.rangedOptimalDistance, {});

    float maxRange = spellInfo->GetMaxRange(false);
    float minRange = spellInfo->GetMinRange(false);
    float optimalRange = (maxRange + minRange) / 2;

    // Position at optimal casting range
    float angle = target->GetRelativeAngle(caster);
    return RotateAroundTarget(target, angle, optimalRange);
}

void DPSPositioning::AvoidFrontalCleaves(Player* dps, Unit* target, float cleaveAngle)
{
    if (!dps || !target)
        return;

    float angleToTarget = target->GetRelativeAngle(dps);
    float targetFacing = target->GetOrientation();
    float angleDiff = ::std::abs(Position::NormalizeOrientation(angleToTarget - targetFacing));

    // Check if in cleave danger zone
    if (angleDiff < cleaveAngle / 2)
    {
        // Move out of cleave range
        float safeAngle = Position::NormalizeOrientation(
            targetFacing + (angleToTarget > targetFacing ? cleaveAngle/2 + 0.2f : -cleaveAngle/2 - 0.2f));
        Position safePos = RotateAroundTarget(target, safeAngle, ::std::sqrt(dps->GetExactDistSq(target)));

        // PHASE 6B: Use Movement Arbiter with ROLE_POSITIONING priority (170)
        BotAI* botAI = dynamic_cast<BotAI*>(dps->GetAI());
        if (botAI && botAI->GetUnifiedMovementCoordinator())
        {
            bool accepted = botAI->RequestPointMovement(
                PlayerBotMovementPriority::ROLE_POSITIONING,
                safePos,
                "DPS safe positioning",
                "RoleBasedCombatPositioning");

            if (!accepted)
            {
                TC_LOG_TRACE("playerbot.movement.arbiter",
                    "RoleBasedCombatPositioning: DPS safe positioning rejected for {} - higher priority active",
                    dps->GetName());
            }
        }
        else
        {
            // FALLBACK: Use BotMovementController with validated pathfinding
            if (BotAI* ai = GetBotAI(dps))
            {
                if (!ai->MoveTo(safePos, true))
                {
                    // Final fallback to legacy if validation fails
                    dps->GetMotionMaster()->MovePoint(0, safePos);
                }
            }
            else
            {
                // Non-bot player - use standard movement
                dps->GetMotionMaster()->MovePoint(0, safePos);
            }
        }
    }
}

void DPSPositioning::AvoidTailSwipe(Player* dps, Unit* target, float swipeAngle)
{
    if (!dps || !target)
        return;

    float angleToTarget = target->GetRelativeAngle(dps);
    float targetRear = Position::NormalizeOrientation(target->GetOrientation() + static_cast<float>(M_PI));
    float angleDiff = ::std::abs(Position::NormalizeOrientation(angleToTarget - targetRear));

    // Check if in tail swipe danger zone
    if (angleDiff < swipeAngle / 2)
    {
        // Move to flank
        Position flankPos = CalculateFlankPosition(target, angleToTarget > targetRear);
        // PHASE 6B: Use Movement Arbiter with ROLE_POSITIONING priority (170)
        BotAI* botAI = dynamic_cast<BotAI*>(dps->GetAI());
        if (botAI && botAI->GetUnifiedMovementCoordinator())
        {
            bool accepted = botAI->RequestPointMovement(
                PlayerBotMovementPriority::ROLE_POSITIONING,
                flankPos,
                "DPS flank positioning",
                "RoleBasedCombatPositioning");

            if (!accepted)
            {
                TC_LOG_TRACE("playerbot.movement.arbiter",
                    "RoleBasedCombatPositioning: DPS flank positioning rejected for {} - higher priority active",
                    dps->GetName());
            }
        }
        else
        {
            // FALLBACK: Use BotMovementController with validated pathfinding
            if (BotAI* ai = GetBotAI(dps))
            {
                if (!ai->MoveTo(flankPos, true))
                {
                    // Final fallback to legacy if validation fails
                    dps->GetMotionMaster()->MovePoint(0, flankPos);
                }
            }
            else
            {
                // Non-bot player - use standard movement
                dps->GetMotionMaster()->MovePoint(0, flankPos);
            }
        }
    }
}

bool DPSPositioning::IsInCleaveDanger(const Position& pos, Unit* target, float cleaveAngle)
{
    if (!target)
        return false;

    float angleToPos = target->GetRelativeAngle(&pos);
    float targetFacing = target->GetOrientation();
    float angleDiff = ::std::abs(Position::NormalizeOrientation(angleToPos - targetFacing));

    return angleDiff < cleaveAngle / 2;
}

void DPSPositioning::HandlePositionalRequirements(Player* dps, uint32 spellId)
{
    if (!dps)
        return;

    // Check spell positional requirements
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return;

    // Handle specific positional requirements based on spell attributes
    // This would check spell attributes for positional requirements
}

bool DPSPositioning::MeetsPositionalRequirement(Player* dps, Unit* target, PositionalRequirement req)
{
    if (!dps || !target)
        return false;

    float angleToPlayer = target->GetRelativeAngle(dps);
    float targetFacing = target->GetOrientation();
    float angleDiff = Position::NormalizeOrientation(angleToPlayer - targetFacing);

    if (EnumFlag<PositionalRequirement>(req).HasFlag(PositionalRequirement::BEHIND_TARGET))
    {
        // Behind is typically 135-225 degrees from front
        return angleDiff > 2.356f && angleDiff < 3.926f;  // ~135-225 degrees
    }

    if (EnumFlag<PositionalRequirement>(req).HasFlag(PositionalRequirement::FLANK_TARGET))
    {
        // Flank is typically 45-135 or 225-315 degrees
        return (angleDiff > 0.785f && angleDiff < 2.356f) ||
               (angleDiff > 3.926f && angleDiff < 5.497f);
    }

    return true;
}

Position DPSPositioning::FindPositionForRequirement(Unit* target, PositionalRequirement req)
{
    if (!target)
        return {};

    float angle = target->GetOrientation();
    if (EnumFlag<PositionalRequirement>(req).HasFlag(PositionalRequirement::BEHIND_TARGET))
        angle += static_cast<float>(M_PI);
    else if (EnumFlag<PositionalRequirement>(req).HasFlag(PositionalRequirement::FLANK_TARGET))
        angle += static_cast<float>(M_PI) / 2.0f;

    return RotateAroundTarget(target, angle, _config.meleeOptimalDistance);
}

Position DPSPositioning::OptimizeDPSPosition(Player* dps, Unit* target,
                                            const CombatPositionContext& context)
{
    if (!dps || !target)
        return {};

    // Determine if melee or ranged (use squared distance for comparison)
    float currentDistSq = dps->GetExactDistSq(target);
    bool isMelee = currentDistSq <= (MELEE_MAX_DISTANCE * MELEE_MAX_DISTANCE) ||
                   GetOptimalDPSRange(dps, target) <= MELEE_MAX_DISTANCE;

    if (isMelee)
        return CalculateMeleeDPSPosition(target, context.mainTank, context);
    else
        return CalculateRangedDPSPosition(target, _config.rangedOptimalDistance, context);
}

float DPSPositioning::CalculateDPSEfficiency(const Position& pos, Player* dps, Unit* target)
{
    if (!dps || !target)
        return 0.0f;

    float efficiency = 100.0f;

    // Distance efficiency
    float distance = pos.GetExactDist(target->GetPosition());
    float optimalRange = GetOptimalDPSRange(dps, target);
    float rangePenalty = ::std::abs(distance - optimalRange) * 2.0f;
    efficiency -= rangePenalty;

    // Positional efficiency (for classes with positional requirements)
    // This would check class-specific requirements

    return ::std::max(0.0f, efficiency);
}
Position DPSPositioning::CalculateAOEPosition(Player* dps, const ::std::vector<Unit*>& targets)
{
    if (!dps || targets.empty())
        return {};

    // Calculate center of targets for AOE
    float sumX = 0, sumY = 0, sumZ = 0;
    for (const Unit* target : targets)
    {
        sumX += target->GetPositionX();
        sumY += target->GetPositionY();
        sumZ += target->GetPositionZ();
    }

    Position aoeCenter;
    aoeCenter.m_positionX = sumX / targets.size();
    aoeCenter.m_positionY = sumY / targets.size();
    aoeCenter.m_positionZ = sumZ / targets.size();

    // Position at optimal AOE range
    float angle = dps->GetRelativeAngle(&aoeCenter);
    float distance = GetOptimalDPSRange(dps, targets[0]);

    Position aoePos;
    aoePos.m_positionX = aoeCenter.m_positionX + distance * cos(angle);
    aoePos.m_positionY = aoeCenter.m_positionY + distance * sin(angle);
    aoePos.m_positionZ = aoeCenter.m_positionZ;

    return aoePos;
}

Position DPSPositioning::CalculateCleaveDPSPosition(Player* dps, const ::std::vector<Unit*>& targets)
{
    if (!dps || targets.size() < 2)
        return {};

    // Position to hit multiple targets with cleave
    // Find position where targets are lined up
    Position cleavePos;

    // Simple approach: position perpendicular to line between first two targets
    Unit* target1 = targets[0];
    Unit* target2 = targets[1];

    float angleBetween = target1->GetRelativeAngle(target2);
    float cleaveAngle = Position::NormalizeOrientation(angleBetween + static_cast<float>(M_PI)/2.0f);

    cleavePos.m_positionX = target1->GetPositionX() + _config.meleeOptimalDistance * cos(cleaveAngle);
    cleavePos.m_positionY = target1->GetPositionY() + _config.meleeOptimalDistance * sin(cleaveAngle);
    cleavePos.m_positionZ = target1->GetPositionZ();
    return cleavePos;
}

::std::vector<RolePositionScore> DPSPositioning::EvaluateDPSPositions(
    const ::std::vector<Position>& candidates, Player* dps, const CombatPositionContext& context)
{
    ::std::vector<RolePositionScore> scores;
    scores.reserve(candidates.size());

    for (const Position& pos : candidates)
    {
        RolePositionScore score;
        score.position = pos;
        score.isValid = ValidateDPSPosition(pos, dps, context);

        if (score.isValid)
        {
            score.roleScore = CalculateDPSScore(pos, dps, context);
            score.mechanicScore = IsInCleaveDanger(pos, context.primaryTarget, context.cleaveAngle) ? 0.0f : 100.0f;
            score.safetyScore = 75.0f;  // DPS has moderate safety priority
            score.efficiencyScore = CalculateDPSEfficiency(pos, dps, context.primaryTarget);
            score.mobilityScore = 60.0f;  // DPS needs moderate mobility

            score.totalScore = score.roleScore * 0.25f + score.mechanicScore * 0.25f +
                              score.safetyScore * 0.1f + score.efficiencyScore * 0.35f +
                              score.mobilityScore * 0.05f;
        }

        scores.push_back(score);
    }

    return scores;
}

float DPSPositioning::CalculateDPSScore(const Position& pos, Player* dps,
                                       const CombatPositionContext& context)
{
    if (!dps || !context.primaryTarget)
        return 0.0f;

    float score = 100.0f;

    // Range score
    float distance = pos.GetExactDist(context.primaryTarget->GetPosition());
    float optimalRange = GetOptimalDPSRange(dps, context.primaryTarget);
    float rangeScore = 100.0f - ::std::abs(distance - optimalRange) * 5.0f;
    score = rangeScore * 0.4f;

    // Position score (behind/flank for melee)
    if (distance <= MELEE_MAX_DISTANCE)
    {
        float angleToTarget = context.primaryTarget->GetRelativeAngle(&pos);
        float targetFacing = context.primaryTarget->GetOrientation();
        float angleDiff = Position::NormalizeOrientation(angleToTarget - targetFacing);

        // Behind is best for most melee DPS
    if (angleDiff > 2.356f && angleDiff < 3.926f)
            score += 30.0f;
        // Flank is second best
        else if ((angleDiff > 0.785f && angleDiff < 2.356f) ||
                (angleDiff > 3.926f && angleDiff < 5.497f))
            score += 20.0f;
    }

    // Cleave avoidance
    if (!IsInCleaveDanger(pos, context.primaryTarget, context.cleaveAngle))
        score += 20.0f;

    // Spread maintenance for ranged
    if (distance > MELEE_MAX_DISTANCE && context.requiresSpread)
    {
        float minDistToOther = 999.0f;
        for (const Player* other : context.rangedDPS)
        {
            if (other != dps)
            {
                float dist = pos.GetExactDist(other->GetPosition());
                minDistToOther = ::std::min(minDistToOther, dist);
            }
        }

        if (minDistToOther >= _config.spreadDistance)
            score += 10.0f;
    }

    return ::std::min(100.0f, score);
}

bool DPSPositioning::ValidateDPSPosition(const Position& pos, Player* dps,
                                        const CombatPositionContext& context)
{
    if (!dps || !context.primaryTarget)
        return false;

    float distance = pos.GetExactDist(context.primaryTarget->GetPosition());

    // Check range requirements
    float optimalRange = GetOptimalDPSRange(dps, context.primaryTarget);
    if (distance > optimalRange * 1.5f)
        return false;

    // Check cleave safety
    if (IsInCleaveDanger(pos, context.primaryTarget, context.cleaveAngle))
        return false;

    return true;
}

float DPSPositioning::GetOptimalDPSRange(Player* dps, Unit* target)
{
    if (!dps || !target)
        return _config.rangedOptimalDistance;

    // Check if melee class/spec
    Classes playerClass = static_cast<Classes>(dps->GetClass());
    switch (playerClass)
    {
        case CLASS_WARRIOR:
        case CLASS_ROGUE:
        case CLASS_DEATH_KNIGHT:
        case CLASS_MONK:
        case CLASS_DEMON_HUNTER:
            return _config.meleeOptimalDistance;
        case CLASS_HUNTER:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
        case CLASS_PRIEST:
            return _config.rangedOptimalDistance;
        case CLASS_PALADIN:
        case CLASS_SHAMAN:
        case CLASS_DRUID:
        case CLASS_EVOKER:
            // These classes can be either melee or ranged depending on spec
            // Would need to check specialization
            return _config.rangedOptimalDistance;
        default:
            return _config.rangedOptimalDistance;
    }
}

Position DPSPositioning::RotateAroundTarget(Unit* target, float angle, float distance)
{
    if (!target)
        return {};

    Position pos;
    pos.m_positionX = target->GetPositionX() + distance * cos(angle);
    pos.m_positionY = target->GetPositionY() + distance * sin(angle);
    pos.m_positionZ = target->GetPositionZ();
    return pos;
}

// RoleBasedCombatPositioning Implementation
RoleBasedCombatPositioning::RoleBasedCombatPositioning()
    : _tankPositioning(::std::make_unique<TankPositioning>()),
      _healerPositioning(::std::make_unique<HealerPositioning>()),
      _dpsPositioning(::std::make_unique<DPSPositioning>()),
      _positionManager(nullptr),
      _threatManager(nullptr),
      _formationManager(nullptr)
{
}

void RoleBasedCombatPositioning::Initialize(PositionManager* positionMgr,
                                           BotThreatManager* threatMgr,
                                           FormationManager* formationMgr)
{
    _positionManager = positionMgr;
    _threatManager = threatMgr;
    _formationManager = formationMgr;
}

Position RoleBasedCombatPositioning::CalculateCombatPosition(Player* bot,
                                                            const CombatPositionContext& context)
{
    if (!bot)
        return {};

    auto start = ::std::chrono::high_resolution_clock::now();

    // Determine role if not specified
    ThreatRole role = context.role;
    if (role == ThreatRole::UNDEFINED)
        role = DetermineRole(bot);

    // Calculate position based on role
    Position targetPos = CalculateRolePosition(bot, role, context);

    // Track performance
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::high_resolution_clock::now() - start);
    TrackCalculationTime(duration);

    return targetPos;
}

PositionMovementResult RoleBasedCombatPositioning::UpdateCombatPosition(Player* bot,
                                                               const CombatPositionContext& context)
{
    if (!bot || !_positionManager)
        return {};

    Position targetPos = CalculateCombatPosition(bot, context);

    // Use position manager for movement execution
    MovementContext moveContext;
    moveContext.bot = bot;
    moveContext.target = context.primaryTarget;
    moveContext.primaryThreat = context.currentThreat;
    moveContext.desiredType = PositionType::FORMATION;
    moveContext.botRole = context.role;
    moveContext.inCombat = context.inCombat;
    moveContext.emergencyMode = context.hasActiveAOE;

    return _positionManager->ExecuteMovement(targetPos, MovementPriority::PRIORITY_CRITICAL);
}

Position RoleBasedCombatPositioning::CalculateRolePosition(Player* bot, ThreatRole role,
                                                          const CombatPositionContext& context)
{
    switch (role)
    {
        case ThreatRole::TANK:
            return CalculateTankPosition(bot, context);
        case ThreatRole::HEALER:
            return CalculateHealerPosition(bot, context);
        case ThreatRole::DPS:
            return CalculateDPSPosition(bot, context);
        case ThreatRole::SUPPORT:
            // Support uses flexible positioning
            return CalculateDPSPosition(bot, context);
        default:
            return bot->GetPosition();
    }
}

Position RoleBasedCombatPositioning::CalculateTankPosition(Player* tank,
                                                          const CombatPositionContext& context)
{
    if (!tank || !context.primaryTarget)
        return tank ? tank->GetPosition() : Position();

    // Use tank positioning system
    return _tankPositioning->CalculateTankPosition(context.primaryTarget, context.group, context);
}

Position RoleBasedCombatPositioning::CalculateHealerPosition(Player* healer,
                                                            const CombatPositionContext& context)
{
    if (!healer)
        return {};

    // Use healer positioning system
    return _healerPositioning->CalculateHealerPosition(context.group, context.primaryTarget, context);
}

Position RoleBasedCombatPositioning::CalculateDPSPosition(Player* dps,
                                                         const CombatPositionContext& context)
{
    if (!dps || !context.primaryTarget)
        return dps ? dps->GetPosition() : Position();

    // Use DPS positioning system
    return _dpsPositioning->OptimizeDPSPosition(dps, context.primaryTarget, context);
}

CombatPositionStrategy RoleBasedCombatPositioning::SelectStrategy(const CombatPositionContext& context)
{
    // Select strategy based on role and combat situation
    switch (context.role)
    {
        case ThreatRole::TANK:
            return context.isTankSwap ? CombatPositionStrategy::TANK_ROTATE :
                                       CombatPositionStrategy::TANK_FRONTAL;

        case ThreatRole::HEALER:
            return context.hasActiveAOE ? CombatPositionStrategy::HEALER_SAFE :
                                         CombatPositionStrategy::HEALER_CENTRAL;

        case ThreatRole::DPS:
        {
            if (context.bot)
            {
                float distSq = context.primaryTarget ?
                    context.bot->GetExactDistSq(context.primaryTarget) : 0.0f;

                if (distSq <= (5.0f * 5.0f))  // Melee range (squared)
                    return context.cleaveAngle > 0 ? CombatPositionStrategy::MELEE_FLANK :
                                                    CombatPositionStrategy::MELEE_BEHIND;
                else  // Ranged
                    return context.requiresSpread ? CombatPositionStrategy::RANGED_SPREAD :
                                                   CombatPositionStrategy::RANGED_STACK;
            }
            break;
        }

        case ThreatRole::SUPPORT:
            return CombatPositionStrategy::SUPPORT_FLEXIBLE;

        default:
            break;
    }

    return CombatPositionStrategy::SUPPORT_FLEXIBLE;
}

void RoleBasedCombatPositioning::UpdateStrategy(Player* bot, CombatPositionStrategy newStrategy)
{
    if (!bot)
        return;

    ::std::lock_guard lock(_mutex);
    _strategyCache[bot->GetGUID()] = newStrategy;
    _lastStrategyUpdate[bot->GetGUID()] = GameTime::GetGameTimeMS();
}

PositionalRequirement RoleBasedCombatPositioning::GetPositionalRequirements(Player* bot, Unit* target)
{
    if (!bot || !target)
        return PositionalRequirement::NONE;

    PositionalRequirement requirements = PositionalRequirement::NONE;

    // Determine role-based requirements
    ThreatRole role = DetermineRole(bot);

    switch (role)
    {
        case ThreatRole::TANK:
            requirements = PositionalRequirement::TANK_REQUIREMENTS;
            break;
        case ThreatRole::HEALER:
            requirements = PositionalRequirement::HEALER_REQUIREMENTS;
            break;
        case ThreatRole::DPS:
        {
            float distSq = bot->GetExactDistSq(target);
            if (distSq <= (5.0f * 5.0f)) // 25.0f
                requirements = PositionalRequirement::MELEE_DPS_REQUIREMENTS;
            else
                requirements = PositionalRequirement::RANGED_DPS_REQUIREMENTS;
            break;
        }
        default:
            requirements = PositionalRequirement::LOS_REQUIRED;
            break;
    }

    return requirements;
}

bool RoleBasedCombatPositioning::ValidatePositionalRequirements(const Position& pos,
                                                               PositionalRequirement requirements,
                                                               const CombatPositionContext& context)
{
    if (requirements == PositionalRequirement::NONE)
        return true;

    // Validate each requirement
    if (EnumFlag<PositionalRequirement>(requirements).HasFlag(PositionalRequirement::BEHIND_TARGET) && context.primaryTarget)
    {
        float angle = context.primaryTarget->GetRelativeAngle(&pos);
        float facing = context.primaryTarget->GetOrientation();
        float diff = Position::NormalizeOrientation(angle - facing);
        if (diff < 2.356f || diff > 3.926f)  // Not behind
            return false;
    }

    if (EnumFlag<PositionalRequirement>(requirements).HasFlag(PositionalRequirement::AVOID_FRONTAL) && context.primaryTarget)
    {
        float angle = context.primaryTarget->GetRelativeAngle(&pos);
        float facing = context.primaryTarget->GetOrientation();
        float diff = ::std::abs(Position::NormalizeOrientation(angle - facing));
        if (diff < static_cast<float>(M_PI) / 3.0f)  // In frontal cone
            return false;
    }

    // Additional requirement checks...

    return true;
}

void RoleBasedCombatPositioning::CoordinateGroupPositioning(Group* group, Unit* target)
{
    if (!group || !target)
        return;

    CombatPositionContext context = AnalyzeCombatContext(nullptr, group);
    context.primaryTarget = target;
    UpdateGroupRoles(group, context);

    // Calculate positions for each role group
    for (Player* tank : context.tanks)
    {
        Position pos = CalculateTankPosition(tank, context);
        if (BotAI* ai = GetBotAI(tank))
        {
            if (!ai->MoveTo(pos, true))
            {
                // Fallback to legacy if validation fails
                tank->GetMotionMaster()->MovePoint(0, pos);
            }
        }
        else
        {
            // Non-bot player - use standard movement
            tank->GetMotionMaster()->MovePoint(0, pos);
        }
    }

    for (Player* healer : context.healers)
    {
        Position pos = CalculateHealerPosition(healer, context);
        if (BotAI* ai = GetBotAI(healer))
        {
            if (!ai->MoveTo(pos, true))
            {
                // Fallback to legacy if validation fails
                healer->GetMotionMaster()->MovePoint(0, pos);
            }
        }
        else
        {
            // Non-bot player - use standard movement
            healer->GetMotionMaster()->MovePoint(0, pos);
        }
    }

    // Distribute melee DPS
    if (!context.meleeDPS.empty())
        _dpsPositioning->DistributeMeleePositions(context.meleeDPS, target, context.mainTank);

    // Spread ranged DPS
    if (!context.rangedDPS.empty())
        _dpsPositioning->SpreadRangedPositions(context.rangedDPS, target, 8.0f);
}

void RoleBasedCombatPositioning::OptimizeRoleDistribution(Group* group)
{
    if (!group)
        return;

    // Analyze group composition and optimize role assignments
    // This would involve checking specs, gear, and performance
}

::std::unordered_map<ObjectGuid, Position> RoleBasedCombatPositioning::CalculateGroupFormation(
    Group* group, Unit* target)
{
    ::std::unordered_map<ObjectGuid, Position> formation;

    if (!group || !target)
        return formation;

    CombatPositionContext context = AnalyzeCombatContext(nullptr, group);
    context.primaryTarget = target;
    UpdateGroupRoles(group, context);

    // Calculate position for each member
    for (GroupReference const& ref : group->GetMembers())
    {
        if (Player* member = ref.GetSource())
        {
            if (member->IsAlive())
            {
                context.bot = member;
                Position pos = CalculateCombatPosition(member, context);
                formation[member->GetGUID()] = pos;
            }
        }
    }

    return formation;
}

void RoleBasedCombatPositioning::AdjustForMechanics(Player* bot, const ::std::vector<Position>& dangerZones)
{
    if (!bot || dangerZones.empty())
        return;

    Position currentPos = bot->GetPosition();

    // Check if current position is in danger
    for (const Position& danger : dangerZones)
    {
        if (currentPos.GetExactDist(danger) < 10.0f)  // 10 yard danger radius
        {
            // Find safe position
            RespondToEmergency(bot, danger);
            break;
        }
    }
}

void RoleBasedCombatPositioning::RespondToEmergency(Player* bot, const Position& safeZone)
{
    if (!bot)
        return;

    // Move to safe position immediately
    // PHASE 6B: Use Movement Arbiter with ROLE_POSITIONING priority (170)
        BotAI* botAI = dynamic_cast<BotAI*>(bot->GetAI());
        if (botAI && botAI->GetUnifiedMovementCoordinator())
        {
            bool accepted = botAI->RequestPointMovement(
                PlayerBotMovementPriority::ROLE_POSITIONING,
                safeZone,
                "Safe zone emergency positioning",
                "RoleBasedCombatPositioning");

            if (!accepted)
            {
                TC_LOG_TRACE("playerbot.movement.arbiter",
                    "RoleBasedCombatPositioning: Safe zone emergency positioning rejected for {} - higher priority active",
                    bot->GetName());
            }
        }
        else
        {
            // FALLBACK: Use BotMovementController with validated pathfinding
            // Note: Emergency movement prioritizes safety over validation
            if (BotAI* ai = GetBotAI(bot))
            {
                if (!ai->MoveTo(safeZone, true))
                {
                    // Final fallback to legacy if validation fails
                    bot->GetMotionMaster()->MovePoint(0, safeZone);
                }
            }
            else
            {
                // Non-bot player - use standard movement
                bot->GetMotionMaster()->MovePoint(0, safeZone);
            }
        }  // High priority movement

    // _emergencyMoves++; // Member variable not declared in header
}

float RoleBasedCombatPositioning::GetAverageCalculationTime() const
{
    if (_calculationCount == 0)
        return 0.0f;

    return _averageCalculationTime.count() / 1000.0f;  // Convert to milliseconds
}

uint32 RoleBasedCombatPositioning::GetPositionUpdateCount() const
{
    return _positionUpdates.load();
}

void RoleBasedCombatPositioning::ResetPerformanceMetrics()
{
    _positionUpdates = 0;
    _calculationCount = 0;
    _totalCalculationTime = ::std::chrono::microseconds{0};
    _averageCalculationTime = ::std::chrono::microseconds{0};
}

void RoleBasedCombatPositioning::SetTankConfig(const TankPositionConfig& config)
{
    _tankPositioning->SetConfig(config);
}

void RoleBasedCombatPositioning::SetHealerConfig(const HealerPositionConfig& config)
{
    _healerPositioning->SetConfig(config);
}

void RoleBasedCombatPositioning::SetDPSConfig(const DPSPositionConfig& config)
{
    _dpsPositioning->SetConfig(config);
}

ThreatRole RoleBasedCombatPositioning::DetermineRole(Player* bot)
{
    if (!bot)
        return ThreatRole::UNDEFINED;

    // =========================================================================
    // Full specialization-based role detection for TWW 12.0
    // Uses ChrSpecialization enum for precise role determination
    // =========================================================================

    // 1. Get player's active specialization
    ChrSpecialization spec = bot->GetPrimarySpecialization();

    // 2. Check for explicit group role assignment first (if available)
    Group* group = bot->GetGroup();
    if (group)
    {
        uint8 groupRole = group->GetLfgRoles(bot->GetGUID());
        if (groupRole & lfg::PLAYER_ROLE_TANK)
            return ThreatRole::TANK;
        if (groupRole & lfg::PLAYER_ROLE_HEALER)
            return ThreatRole::HEALER;
        if (groupRole & lfg::PLAYER_ROLE_DAMAGE)
            return ThreatRole::DPS;
    }

    // 3. Specialization-based role determination
    // TWW 12.0 specialization IDs from ChrSpecialization enum
    switch (spec)
    {
        // =====================================================================
        // TANK SPECIALIZATIONS
        // =====================================================================
        case ChrSpecialization(73):    // Protection Warrior
        case ChrSpecialization(66):    // Protection Paladin
        case ChrSpecialization(250):   // Blood Death Knight
        case ChrSpecialization(104):   // Guardian Druid
        case ChrSpecialization(268):   // Brewmaster Monk
        case ChrSpecialization(581):   // Vengeance Demon Hunter
            return ThreatRole::TANK;

        // =====================================================================
        // HEALER SPECIALIZATIONS
        // =====================================================================
        case ChrSpecialization(256):   // Discipline Priest
        case ChrSpecialization(257):   // Holy Priest
        case ChrSpecialization(65):    // Holy Paladin
        case ChrSpecialization(264):   // Restoration Shaman
        case ChrSpecialization(105):   // Restoration Druid
        case ChrSpecialization(270):   // Mistweaver Monk
        case ChrSpecialization(1468):  // Preservation Evoker
            return ThreatRole::HEALER;

        // =====================================================================
        // MELEE DPS SPECIALIZATIONS
        // =====================================================================
        case ChrSpecialization(71):    // Arms Warrior
        case ChrSpecialization(72):    // Fury Warrior
        case ChrSpecialization(259):   // Assassination Rogue
        case ChrSpecialization(260):   // Outlaw Rogue
        case ChrSpecialization(261):   // Subtlety Rogue
        case ChrSpecialization(251):   // Frost Death Knight
        case ChrSpecialization(252):   // Unholy Death Knight
        case ChrSpecialization(255):   // Survival Hunter
        case ChrSpecialization(263):   // Enhancement Shaman
        case ChrSpecialization(269):   // Windwalker Monk
        case ChrSpecialization(70):    // Retribution Paladin
        case ChrSpecialization(103):   // Feral Druid
        case ChrSpecialization(577):   // Havoc Demon Hunter
        case ChrSpecialization(1473):  // Augmentation Evoker (melee support)
            return ThreatRole::DPS;

        // =====================================================================
        // RANGED DPS SPECIALIZATIONS
        // =====================================================================
        case ChrSpecialization(253):   // Beast Mastery Hunter
        case ChrSpecialization(254):   // Marksmanship Hunter
        case ChrSpecialization(258):   // Shadow Priest
        case ChrSpecialization(262):   // Elemental Shaman
        case ChrSpecialization(62):    // Arcane Mage
        case ChrSpecialization(63):    // Fire Mage
        case ChrSpecialization(64):    // Frost Mage
        case ChrSpecialization(265):   // Affliction Warlock
        case ChrSpecialization(266):   // Demonology Warlock
        case ChrSpecialization(267):   // Destruction Warlock
        case ChrSpecialization(102):   // Balance Druid
        case ChrSpecialization(1467):  // Devastation Evoker
            return ThreatRole::DPS;

        default:
            break;
    }

    // 4. Fallback: Skip gear analysis (deprecated ITEM_MOD_* constants removed in TWW 12.0)
    // In modern WoW, specialization is always set, so gear analysis is rarely needed
    // If specialization is unset, fall back directly to class-based defaults

    // 5. Check active auras for role indicators (tank stance/presence spells)
    // Defensive Stance (Warrior tank stance)
    if (bot->HasAura(71))
        return ThreatRole::TANK;
    // Blood Presence (DK tank)
    if (bot->HasAura(48263))
        return ThreatRole::TANK;
    // Bear Form (Druid tank)
    if (bot->HasAura(5487) || bot->HasAura(9634))
        return ThreatRole::TANK;

    // 6. Final fallback to class-based defaults for pure classes
    Classes playerClass = static_cast<Classes>(bot->GetClass());
    switch (playerClass)
    {
        case CLASS_ROGUE:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
        case CLASS_HUNTER:
            return ThreatRole::DPS;

        case CLASS_PRIEST:
            return ThreatRole::HEALER; // Default to healer for unspecialized priests

        case CLASS_WARRIOR:
        case CLASS_PALADIN:
        case CLASS_DEATH_KNIGHT:
            return ThreatRole::TANK; // Default to tank for plate wearers

        case CLASS_DRUID:
        case CLASS_SHAMAN:
        case CLASS_MONK:
        case CLASS_DEMON_HUNTER:
        case CLASS_EVOKER:
            return ThreatRole::DPS; // Hybrid classes default to DPS

        default:
            return ThreatRole::UNDEFINED;
    }
}

bool RoleBasedCombatPositioning::IsRoleCompatible(ThreatRole role, CombatPositionStrategy strategy)
{
    switch (role)
    {
        case ThreatRole::TANK:
            return strategy == CombatPositionStrategy::TANK_FRONTAL ||
                   strategy == CombatPositionStrategy::TANK_ROTATE;

        case ThreatRole::HEALER:
            return strategy == CombatPositionStrategy::HEALER_CENTRAL ||
                   strategy == CombatPositionStrategy::HEALER_SAFE;

        case ThreatRole::DPS:
            return strategy == CombatPositionStrategy::MELEE_BEHIND ||
                   strategy == CombatPositionStrategy::MELEE_FLANK ||
                   strategy == CombatPositionStrategy::RANGED_SPREAD ||
                   strategy == CombatPositionStrategy::RANGED_STACK;

        default:
            return true;
    }
}

float RoleBasedCombatPositioning::CalculateRoleEfficiency(Player* bot, ThreatRole role,
                                                         const Position& pos)
{
    if (!bot)
        return 0.0f;

    // Calculate efficiency based on role requirements at position
    float efficiency = 100.0f;

    // Role-specific efficiency calculations would go here

    return efficiency;
}

Position RoleBasedCombatPositioning::CalculatePositionByStrategy(Player* bot,
                                                                CombatPositionStrategy strategy,
                                                                const CombatPositionContext& context)
{
    if (!bot)
        return {};

    switch (strategy)
    {
        case CombatPositionStrategy::TANK_FRONTAL:
            return _tankPositioning->CalculateTankPosition(context.primaryTarget, context.group, context);

        case CombatPositionStrategy::TANK_ROTATE:
            return _tankPositioning->CalculateOffTankPosition(context.primaryTarget, context.mainTank, context);

        case CombatPositionStrategy::HEALER_CENTRAL:
            return _healerPositioning->CalculateHealerPosition(context.group, context.primaryTarget, context);

        case CombatPositionStrategy::HEALER_SAFE:
            return _healerPositioning->FindSafeHealingSpot(bot, context.primaryTarget, context);

        case CombatPositionStrategy::MELEE_BEHIND:
            return _dpsPositioning->CalculateBackstabPosition(context.primaryTarget);

        case CombatPositionStrategy::MELEE_FLANK:
            return _dpsPositioning->CalculateFlankPosition(context.primaryTarget, true);

        case CombatPositionStrategy::RANGED_SPREAD:
        case CombatPositionStrategy::RANGED_STACK:
            return _dpsPositioning->CalculateRangedDPSPosition(context.primaryTarget, 25.0f, context);

        default:
            return bot->GetPosition();
    }
}

::std::vector<Position> RoleBasedCombatPositioning::GenerateCandidatePositions(Player* bot,
                                                                            const CombatPositionContext& context)
{
    ::std::vector<Position> candidates;
    if (!bot || !context.primaryTarget)
        return candidates;

    Position center = context.primaryTarget->GetPosition();
    float baseDistance = 5.0f;

    // Generate positions based on role
    ThreatRole role = context.role != ThreatRole::UNDEFINED ? context.role : DetermineRole(bot);

    switch (role)
    {
        case ThreatRole::TANK:
            baseDistance = 3.0f;
            break;
        case ThreatRole::HEALER:
            baseDistance = 25.0f;
            break;
        case ThreatRole::DPS:
            baseDistance = bot->GetExactDistSq(context.primaryTarget) <= (5.0f * 5.0f) ? 3.0f : 25.0f;
            break;
        default:
            baseDistance = 15.0f;
            break;
    }

    // Generate circular positions
    for (int angle = 0; angle < 360; angle += 15)
    {
        float radians = angle * (static_cast<float>(M_PI) / 180.0f);
        Position pos;
        pos.m_positionX = center.m_positionX + baseDistance * cos(radians);
        pos.m_positionY = center.m_positionY + baseDistance * sin(radians);
        pos.m_positionZ = center.m_positionZ;

        candidates.push_back(pos);

        if (candidates.size() >= context.maxCandidates)
            break;
    }

    return candidates;
}

RolePositionScore RoleBasedCombatPositioning::EvaluatePosition(const Position& pos, Player* bot,
                                                              const CombatPositionContext& context)
{
    RolePositionScore score;
    score.position = pos;

    if (!bot || !context.primaryTarget)
        return score;

    ThreatRole role = context.role != ThreatRole::UNDEFINED ? context.role : DetermineRole(bot);

    // Evaluate based on role
    ::std::vector<RolePositionScore> scores;

    switch (role)
    {
        case ThreatRole::TANK:
            scores = _tankPositioning->EvaluateTankPositions({pos}, context);
            break;
        case ThreatRole::HEALER:
            scores = _healerPositioning->EvaluateHealerPositions({pos}, context);
            break;
        case ThreatRole::DPS:
            scores = _dpsPositioning->EvaluateDPSPositions({pos}, bot, context);
            break;
        default:
            break;
    }

    if (!scores.empty())
        score = scores[0];

    return score;
}

CombatPositionContext RoleBasedCombatPositioning::AnalyzeCombatContext(Player* bot, Group* group)
{
    CombatPositionContext context;
    context.bot = bot;
    context.group = group;

    if (group)
    {
        UpdateGroupRoles(group, context);
    }

    // Analyze combat state
    if (bot)
    {
        context.inCombat = bot->IsInCombat();
        context.role = DetermineRole(bot);
    }

    return context;
}

void RoleBasedCombatPositioning::UpdateGroupRoles(Group* group, CombatPositionContext& context)
{
    if (!group)
        return;

    context.tanks.clear();
    context.healers.clear();
    context.meleeDPS.clear();
    context.rangedDPS.clear();

    for (GroupReference const& ref : group->GetMembers())
    {
        if (Player* member = ref.GetSource())
        {
            if (member->IsAlive())
            {
                ThreatRole role = DetermineRole(member);

                switch (role)
                {
                    case ThreatRole::TANK:
                        context.tanks.push_back(member);
                        if (!context.mainTank)
                            context.mainTank = member;
                        else if (!context.offTank)
                            context.offTank = member;
                        break;

                    case ThreatRole::HEALER:
                        context.healers.push_back(member);
                        break;

                    case ThreatRole::DPS:
                    {
                        // Determine if melee or ranged (use squared distance)
                        float distSq = context.primaryTarget ?
                            member->GetExactDistSq(context.primaryTarget) : (30.0f * 30.0f);

                        if (distSq <= (5.0f * 5.0f)) // 25.0f
                            context.meleeDPS.push_back(member);
                        else
                            context.rangedDPS.push_back(member);
                        break;
                    }

                    default:
                        break;
                }
            }
        }
    }
}

void RoleBasedCombatPositioning::IdentifyDangerZones(Unit* target, CombatPositionContext& context)
{
    if (!target)
        return;

    context.dangerZones.clear();

    // Add frontal cleave zone if applicable
    if (context.cleaveAngle > 0)
    {
        Position cleaveZone;
        cleaveZone.m_positionX = target->GetPositionX() + 5.0f * cos(target->GetOrientation());
        cleaveZone.m_positionY = target->GetPositionY() + 5.0f * sin(target->GetOrientation());
        cleaveZone.m_positionZ = target->GetPositionZ();
        context.dangerZones.push_back(cleaveZone);
    }

    // Add tail swipe zone if applicable
    if (context.tailSwipeAngle > 0)
    {
        float rearAngle = Position::NormalizeOrientation(target->GetOrientation() + static_cast<float>(M_PI));
        Position tailZone;
        tailZone.m_positionX = target->GetPositionX() + 5.0f * cos(rearAngle);
        tailZone.m_positionY = target->GetPositionY() + 5.0f * sin(rearAngle);
        tailZone.m_positionZ = target->GetPositionZ();
        context.dangerZones.push_back(tailZone);
    }

    // Additional mechanic-specific danger zones would be added here
}

void RoleBasedCombatPositioning::TrackCalculationTime(::std::chrono::microseconds duration)
{
    _calculationCount++;
    _totalCalculationTime += duration;

    uint32 count = _calculationCount.load();
    if (count > 0)
    {
        _averageCalculationTime = _totalCalculationTime / count;
    }
}

} // namespace Playerbot